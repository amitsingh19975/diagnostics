#ifndef AMT_DARK_DIAGNOSTICS_RENDERER_HPP
#define AMT_DARK_DIAGNOSTICS_RENDERER_HPP

#include "core/term/canvas.hpp"
#include "basic.hpp"
#include "core/term/terminal.hpp"
#include "core/format_any.hpp"
#include "core/term/annotated_string.hpp"
#include "core/term/color.hpp"
#include "core/config.hpp"
#include "core/cow_string.hpp"
#include "core/small_vec.hpp"
#include "core/utf8.hpp"
#include "core/term/style.hpp"
#include "span.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <print>
#include <queue>
#include <string_view>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dark {
    struct Markers {
        std::string_view primary   = "^";
        std::string_view quad      = "≣";
        std::string_view tripple   = "≋";
        std::string_view double_   = "≈";
        std::string_view single    = "~";
        std::string_view remove    = "x";
        std::string_view insert    = "+";
    };

    struct DiagnosticRenderConfig {
        term::BoxCharSet box_normal{ term::char_set::box::rounded };
        term::BoxCharSet box_bold{ term::char_set::box::rounded_bold };
        term::LineCharSet line_normal{ term::char_set::line::rounded };
        term::LineCharSet line_bold{ term::char_set::line::rounded_bold };
        term::ArrowCharSet arrow_normal{ term::char_set::arrow::basic };
        term::ArrowCharSet arrow_bold{ term::char_set::arrow::basic_bold };
        std::string_view dotted_vertical{ term::char_set::line::dotted.vertical };
        std::string_view dotted_horizontal{ term::char_set::line::dotted.horizonal };
        std::string_view bullet_point = "●";
        std::string_view square = "◼";
        Markers markers{};
        std::size_t max_message_characters_per_line{60};
        unsigned max_non_marker_lines{4};
        unsigned diagnostic_kind_padding{4};
        Color ruler_color{Color::Magenta};
        std::array<Color, diagnostic_level_elements_count> level_to_color{
            /*Help   */ Color::Green,
            /*Note   */ Color::Blue,
            /*Warning*/ Color::Yellow,
            /*Error  */ Color::Red,
            /*Insert */ Color::BrightGreen,
            /*Delete */ Color::BrightRed
        };
    };
} // namespace dark

namespace dark::internal {
    struct GroupId {
        static constexpr unsigned diagnostic_source = 1;
        static constexpr unsigned diagnostic_ruler = 2;
        static constexpr unsigned diagnostic_annotation_base_offset = 3;
        static constexpr unsigned diagnostic_orphan_message = 4;
        static constexpr unsigned diagnostic_message = 5;
        static constexpr unsigned diagnostic_path = 500; // must be the highest value;
    };

    template <core::IsFormattable T>
    auto convert_diagnostic_kind_to_string(T kind, unsigned padding) -> std::string {
        if constexpr (std::convertible_to<T, std::size_t>) {
            return std::format("{:0{}}", static_cast<std::size_t>(kind), padding);
        } else {
            return std::format("{}", core::FormatterAnyArg(kind));
        }
    }

    template <typename T>
    auto convert_diagnostic_kind_to_string([[maybe_unused]] T kind, unsigned padding) -> std::string {
        if constexpr (std::convertible_to<T, std::size_t>) {
            return std::format("{:0{}}", static_cast<std::size_t>(kind), padding);
        } else {
            return {};
        }
    }

    constexpr auto diagnostic_level_to_color(std::span<Color> colors, DiagnosticLevel level) noexcept -> Color {
        return colors[static_cast<std::size_t>(level)];
    }

    constexpr auto diagnostic_level_to_color(std::span<const Color> colors, DiagnosticLevel level) noexcept -> Color {
        return colors[static_cast<std::size_t>(level)];
    }

    constexpr auto diagnostic_level_code_prefix(DiagnosticLevel level) noexcept -> std::string_view {
        switch (level) {
        case DiagnosticLevel::Help: return "H";
        case DiagnosticLevel::Note: return "N";
        case DiagnosticLevel::Warning: return "W";
        case DiagnosticLevel::Error: return "E";
        case DiagnosticLevel::Insert: return "I";
        case DiagnosticLevel::Delete: return "D";
          break;
        }
    }

    struct DiagnosticMessageSpanInfo {
        static constexpr auto npos = std::numeric_limits<std::size_t>::max();
        std::size_t message_index{npos};
        std::size_t diagnostic_index;
        DiagnosticLevel level;
        Span span;
    };

    struct DiagnosticOrphanMessageInfo {
        std::size_t message_index{};
        DiagnosticLevel level;
    };

    struct NormalizedDiagnosticAnnotations {
        using diagnostic_index_t = std::size_t;
        core::SmallVec<term::AnnotatedString> messages{};
        core::SmallVec<DiagnosticMessageSpanInfo> spans{};
        std::unordered_map<diagnostic_index_t, DiagnosticSourceLocationTokens> tokens{};
        core::SmallVec<DiagnosticOrphanMessageInfo> orphans{};

        static constexpr auto compare_annotated_string(
            term::AnnotatedString const& lhs,
            term::AnnotatedString const& rhs
        ) noexcept -> bool {
            if (lhs.strings.size() != rhs.strings.size()) return false;
            for (auto i = 0ul; i < lhs.strings.size(); ++i) {
                auto const& [l, ls] = lhs.strings[i];
                auto const& [r, rs] = rhs.strings[i];
                if (ls != rs) return false;
                if (l.data() == r.data()) continue;
                if (l.to_borrowed() != r.to_borrowed()) return false;
            }
            return true;
        }
    };

    static inline auto normalize_diagnostic_messages(
        Diagnostic& diag
    ) -> NormalizedDiagnosticAnnotations {
        auto res = NormalizedDiagnosticAnnotations{};
        auto source_span = diag.location.source.span();

        for (auto i = 0ul; i < diag.annotations.size(); ++i) {
            auto annotation = diag.annotations[i];
            auto message_id = DiagnosticMessageSpanInfo::npos;

            // 1. Find unique annotation messages
            for (auto m = 0ul; m < res.messages.size(); ++m) {
                auto const& el = res.messages[m];
                if (NormalizedDiagnosticAnnotations::compare_annotated_string(
                    annotation.message,
                    el
                )) {
                    message_id = m;
                }
            }

            // 2. If not found, insert the current one
            if (message_id == DiagnosticMessageSpanInfo::npos) {
                if (!annotation.message.empty()) {
                    message_id = res.messages.size();
                    res.messages.push_back(std::move(annotation.message));
                }
            }

            // 3. Store tokens inside the hashmap
            res.tokens[i] = std::move(annotation.tokens);
            bool has_spans{false};
            bool inside_source_span{false};

            // 4. Store spans with ids and filter out out-of-bound spans
            for (auto span: annotation.spans) {
                if (!source_span.is_between(span, true)) continue;
                inside_source_span = true;

                if (span.empty()) {
                    continue;
                }

                has_spans = true;

                res.spans.push_back(DiagnosticMessageSpanInfo {
                    .message_index = message_id,
                    .diagnostic_index = i,
                    .level = annotation.level,
                    .span = span
                });
            }

            if ((!has_spans && inside_source_span) || annotation.spans.empty()) {
                res.orphans.push_back({ .message_index = message_id, .level = annotation.level });
            }
        }

        // Sorting required for patching insert tokens
        std::stable_sort(res.spans.begin(), res.spans.end(), [](DiagnosticMessageSpanInfo const& l, DiagnosticMessageSpanInfo const& r) {
            return l.span.start() < r.span.start();
        });
        return res;
    }

    static inline auto render_diagnostic_message(
        term::Canvas& canvas,
        Diagnostic& diag,
        DiagnosticRenderConfig const& config
    ) -> term::BoundingBox {
        auto code = convert_diagnostic_kind_to_string(diag.kind, config.diagnostic_kind_padding);
        auto tag = to_string(diag.level);
        auto span_style = SpanStyle {
            .text_color = diagnostic_level_to_color(std::span(config.level_to_color), diag.level),
            .bold = true,
        };
        auto bbox = term::BoundingBox{};
        if (!code.empty()) {
            auto prefix = diagnostic_level_code_prefix(diag.level);
            bbox = canvas.draw_text(
                AnnotatedString::builder()
                    .with_style(span_style)
                    .push(tag)
                    .push("[").push(prefix).push(core::CowString(code)).push("]: ")
                    .build(),
                bbox.x, bbox.bottom_left().second
            ).bbox;
        } else {
            bbox = canvas.draw_text(
                AnnotatedString::builder()
                    .with_style(span_style)
                    .push(tag).push(": ")
                    .build(),
                bbox.x, bbox.bottom_left().second
            ).bbox;
        }

        auto nbbox = canvas.draw_text(
            diag.message.format().to_borrowed(),
            bbox.width, 0,
            { .group_id = GroupId::diagnostic_message, .word_wrap = true, .break_whitespace = true }
        ).bbox;
        bbox.width = nbbox.width;
        bbox.height = nbbox.height;
        return bbox;
    }

    template <std::integral T>
    DARK_ALWAYS_INLINE static constexpr auto count_digits(T v) noexcept -> std::size_t {
        std::size_t count{0};
        while (v) {
            ++count;
            v /= T(10);
        }
        return std::max(count, std::size_t{1});
    }

    DARK_ALWAYS_INLINE static constexpr auto calculate_max_number_line_width(
        Diagnostic const& diag
    ) noexcept -> std::size_t {
        std::size_t width{};
        // 1. Count from locations
        for (auto const& line: diag.location.source.lines) {
            width = std::max(width, count_digits(line.line_number));
        }

        // 2. Count from annotations
        for (auto const& annotation: diag.annotations) {
            if (!annotation.spans.empty()) continue;
            for (auto const& line: annotation.tokens.lines) {
                width = std::max(width, count_digits(line.line_number));
            }
        }

        return std::max(width + 2, std::size_t{4});
    }

    DARK_ALWAYS_INLINE static auto render_ruler(
        term::Canvas& canvas,
        term::BoundingBox container,
        std::optional<dsize_t> line_number,
        std::string_view vertical,
        std::string_view dotted_vertical,
        Color color
    ) -> term::BoundingBox {
        auto width = container.width;
        auto y = container.y;
        auto line = line_number.has_value() ? vertical : dotted_vertical;
        auto num = line_number
            .transform([width](dsize_t n) -> std::string {
                return std::format("{:>{}}", n, std::max(width, dsize_t{2}) - 2);
            })
            .value_or(std::string());

        canvas.draw_text(num, container.x, y, { .text_color = color, .bold = true, .group_id = GroupId::diagnostic_ruler });
        if (line.empty()) {
            line = "|";
            if (!line_number.has_value()) {
                canvas.draw_text("...", 1, y, { .text_color = color, .bold = true, .group_id = GroupId::diagnostic_ruler });
            }
        }
        canvas.draw_pixel(width, y, line, { .text_color = color, .group_id = GroupId::diagnostic_ruler });
        container.y += 1;
        return container;
    }

    static inline auto render_file_info(
        term::Canvas& canvas,
        Diagnostic const& diag,
        unsigned x,
        unsigned y,
        DiagnosticRenderConfig const& config
    ) noexcept -> term::BoundingBox {
        auto const& loc = diag.location;
        if (loc.filename.empty()) return { .x = x, .y = y, .width = 0, .height = 0 };
        auto [line_num, col_num] = loc.line_info();
        auto turn = config.line_normal.turn_right;
        if (turn.empty()) turn = config.line_normal.vertical;

        auto file_info_style = SpanStyle{ .bold = true };

        auto line_style = SpanStyle {
            .text_color = config.ruler_color
        };

        auto an = AnnotatedString::builder()
            .push(turn, line_style)
            .push(config.line_normal.horizonal, line_style)
            .push("[", line_style)
            .with_style(file_info_style)
                .push(loc.filename)
                .push(core::CowString(std::format(":{}:{}", line_num, col_num)))
            .with_style(line_style)
            .push("]")
            .build();
        auto box = canvas.draw_text(std::move(an), x, y, { .group_id = GroupId::diagnostic_message }).bbox;
        canvas.draw_pixel(
            box.x,
            box.bottom_left().second,
            config.line_normal.vertical,
            { .text_color = *line_style.text_color, .group_id = GroupId::diagnostic_message }
        );
        canvas.draw_pixel(
            box.x,
            box.bottom_left().second + 1,
            config.line_normal.vertical,
            { .text_color = *line_style.text_color, .group_id = GroupId::diagnostic_message }
        );
        box.height += 2;
        return box;
    }

    enum class MarkerKind {
        Primary,
        Secondary,
        Insert,
        Delete,
    };

    constexpr auto mass_from_marker(MarkerKind kind) noexcept -> std::size_t {
        switch (kind) {
        case MarkerKind::Primary: return 3;
        case MarkerKind::Secondary: return 1;
        case MarkerKind::Insert:
        case MarkerKind::Delete: return 2;
        }
    }

    struct DiagnosticMarker {
        MarkerKind kind;
        Span span;
        unsigned annotation_index{};
        bool is_start{true};
    };

    struct NormalizedDiagnosticTokenInfo {
        core::CowString text;
        // The first marker is always the primary marker even if it's
        // empty. So, markers should be be empty.
        core::SmallVec<DiagnosticMarker, 2> markers{};
        dsize_t token_start_offset{};
        Color text_color{ Color::Default };
        Color bg_color{ Color::Default };
        bool bold{false};
        bool italic{false};
        bool is_artificial{false};
        term::TextOverflow overflow{ term::TextOverflow::none };
        Span overflow_span{};

        constexpr auto span() const noexcept -> Span {
            return Span::from_size(token_start_offset, static_cast<dsize_t>(text.size()));
        }

        constexpr auto to_style() const noexcept -> term::Style {
            return {
                .text_color = text_color,
                .bg_color = bg_color,
                .bold = bold,
                .italic = italic,
            };
        }
    };

    struct NormalizedDiagnosticLineTokens {
        core::SmallVec<NormalizedDiagnosticTokenInfo> tokens;
        dsize_t line_number{};
        dsize_t line_start_offset{};

        constexpr auto empty() const noexcept -> bool { return tokens.empty(); }

        friend void swap(NormalizedDiagnosticLineTokens& lhs, NormalizedDiagnosticLineTokens& rhs) {
            using std::swap;
            swap(lhs.tokens, rhs.tokens);
            swap(lhs.line_number, rhs.line_number);
            swap(lhs.line_start_offset, rhs.line_start_offset);
        }
    };


    static inline auto normalize_diagnostic_line(
        DiagnosticLineTokens& line,
        NormalizedDiagnosticAnnotations& an
    ) -> core::SmallVec<NormalizedDiagnosticLineTokens> {
        auto res = core::SmallVec<NormalizedDiagnosticLineTokens>{};
        auto add_entry = [&res](
            dsize_t line_number,
            dsize_t line_start_offset
        ) -> NormalizedDiagnosticLineTokens& {
            res.push_back(NormalizedDiagnosticLineTokens{
                .tokens = {},
                .line_number = line_number,
                .line_start_offset = line_start_offset
            });
            return res.back();
        };
        auto& last = add_entry(line.line_number, line.line_start_offset);
        core::SmallVec<std::size_t, 1> insertion_points;

        for (auto& tok: line.tokens) {
            insertion_points.clear();

            // 1. Find the first match.
            std::size_t start{};
            for (; start < an.spans.size(); ++start) {
                // Since `an.spans` are sorted by start we can use the first match, and from
                // there, we find all the spans that are within the bounds.
                auto const& info = an.spans[start];
                auto span = info.span;
                if (info.level != DiagnosticLevel::Insert) continue;
                if (tok.span().is_between(span.start())) {
                    insertion_points.push_back(start);
                    break;
                }
            }
            // 2. if match not found, we just insert the token with marker. 
            if (start == an.spans.size()) {
                last.tokens.push_back(NormalizedDiagnosticTokenInfo {
                    .text = std::move(tok.text),
                    .markers = { { .kind = MarkerKind::Primary, .span = tok.marker } },
                    .token_start_offset = tok.token_start_offset,
                    .text_color = tok.text_color,
                    .bg_color = tok.bg_color,
                    .bold = tok.bold,
                    .italic = tok.italic
                });
                continue;
            }
            // 3. find the end of the match
            for (; start < an.spans.size(); ++start) {
                auto const& info = an.spans[start];
                auto span = info.span;
                if (!tok.span().is_between(span.start())) break;
                if (info.level != DiagnosticLevel::Insert) continue;
                insertion_points.push_back(start);
            }

            // Case 1:
            //   |--------- Token ----------|
            //      |---- span 1-------|
            //      |--span 2---|
            // Case 2:
            //   |--------- Token ----------|
            //      |---- span 1-------|
            //         |--span 2---|

            auto span = tok.span();
            for (auto i = 0ul; i < insertion_points.size();) {
                auto index = insertion_points[i];
                auto split_point = an.spans[index].span.start();
                auto first = Span::from_size(span.start(), split_point);
                auto end = Span(first.end(), span.end());
                auto start_offset = (tok.marker.start() - tok.token_start_offset);
                auto first_marker = Span::from_size(
                    tok.marker.start(),
                    std::min(tok.marker.size(), std::max(first.size(), start_offset) - start_offset)
                );
                auto last_marker = Span(first_marker.end(), tok.marker.end());

                if (!first.empty()) {
                    last.tokens.push_back(NormalizedDiagnosticTokenInfo {
                        .text = tok.text.substr(0, first.size()),
                        .markers = {},
                        .token_start_offset = tok.token_start_offset,
                        .text_color = tok.text_color,
                        .bg_color = tok.bg_color,
                        .bold = tok.bold,
                        .italic = tok.italic
                    });
                    if (!first_marker.empty()) {
                        last.tokens.back().markers.push_back({
                            .kind = MarkerKind::Primary,
                            .span = first_marker
                        });
                    }
                }

                for (; i < insertion_points.size(); ++i) {
                    index = insertion_points[i];
                    auto& top = an.spans[index];
                    if (top.span.start() != split_point) break;

                    if (auto it = an.tokens.find(top.diagnostic_index); it != an.tokens.end()) {
                        auto lines = std::move(it->second.lines);
                        an.tokens.erase(it);

                        for (auto l = 0ul; l < lines.size(); ++l) {
                            auto& el = lines[l];
                            for (auto& itok: el.tokens) {
                                auto text_size = static_cast<dsize_t>(itok.text.size());
                                if (text_size == 0) continue;

                                last.tokens.push_back(
                                    NormalizedDiagnosticTokenInfo {
                                        .text = std::move(itok.text),
                                        .markers = {
                                            DiagnosticMarker {
                                                .kind = MarkerKind::Insert,
                                                .span = Span::from_size(0, text_size),
                                                .annotation_index = static_cast<unsigned>(index)
                                            }
                                        },
                                        .token_start_offset = tok.token_start_offset,
                                        .text_color = itok.text_color,
                                        .bg_color = itok.bg_color,
                                        .bold = itok.bold,
                                        .italic = itok.italic,
                                        .is_artificial = true
                                    }
                                );
                            }
                            if (l + 1 < lines.size()) {
                                last = add_entry(0, line.line_start_offset);
                            }
                        }
                    }
                }

                if (!end.empty()) {
                    if (i >= insertion_points.size()) {
                        last.tokens.push_back(NormalizedDiagnosticTokenInfo {
                            .text = tok.text.substr(first.size()),
                            .markers = {},
                            .token_start_offset = tok.token_start_offset + first.size(),
                            .text_color = tok.text_color,
                            .bg_color = tok.bg_color,
                            .bold = tok.bold,
                            .italic = tok.italic
                        });

                        if (!last_marker.empty()) {
                            last.tokens.back().markers.push_back({
                                .kind = MarkerKind::Primary,
                                .span = last_marker
                            });
                        }
                    } else {
                        tok.text = tok.text.substr(first.size());
                        tok.marker = last_marker;
                    }
                }
            }
        }

        for (auto& l: res) {
            for (auto i = 0ul; i < l.tokens.size(); ++i) {
                auto& el = l.tokens[i];
                auto span = el.span();
                if (el.is_artificial) continue;
                // Find the intersection spans
                for (auto j = 0ul; j < an.spans.size(); ++j) {
                    auto const& top = an.spans[j];
                    if (top.level == DiagnosticLevel::Insert) continue;
                    if (span.end() <= top.span.start()) break;
                    if (!span.is_between(top.span.start())) continue;

                    // [.........token.........]
                    //    [..first..][..end..]
                    auto first = Span(
                        top.span.start(),
                        std::min(top.span.end(), span.end())
                    );
                    auto end = Span(first.end(), top.span.end());

                    auto kind = MarkerKind::Secondary;
                    if (top.level == DiagnosticLevel::Delete) kind = MarkerKind::Delete;

                    // Insert the first span and overflowing span will be inserted to the next intersecting tokens.
                    el.markers.push_back({
                        .kind = kind,
                        .span = first,
                        .annotation_index = static_cast<unsigned>(j)
                    });

                    // insert the remaining annotation span to the remaining tokens.
                    for (auto k = i + 1; k < l.tokens.size() && !end.empty(); ++k) {
                        auto& tmp = l.tokens[k];
                        if (tmp.is_artificial) continue;
                        if (!tmp.span().is_between(end.start())) break;
                        first = Span(end.start(), std::min(end.end(), tmp.span().end()));
                        end = Span(first.end(), end.end());
                        tmp.markers.push_back({
                            .kind = kind,
                            .span = first,
                            .annotation_index = static_cast<unsigned>(j),
                            .is_start = false
                        });
                    }
                }
            }
        }

        constexpr auto cmp_op = [](DiagnosticMarker const& l, DiagnosticMarker const& r) {
            return l.span.start() < r.span.start();
        };

        for (auto& l: res) {
            for (auto& el: l.tokens) {
                if (el.is_artificial) continue;

                auto size = el.markers.size();
                // 1. [1, 0, 2, 3] -> size = 4
                // 2. [1, 3, 2] -> size = 3
                auto it = std::remove_if(el.markers.begin(), el.markers.end(), [](auto const& m) {
                    return m.span.empty();
                });
                el.markers.erase(it, el.markers.end());

                std::stable_sort(el.markers.begin(), el.markers.end(), cmp_op);

                // remove overlapping spans
                while (true) {
                    auto s = 0ul;
                    auto k = 0ul;
                    bool found = false;
                    size = el.markers.size();
                    for (; s < size; ++s) {
                        auto lhs = el.markers[s];
                        if (lhs.kind == MarkerKind::Insert) continue;
                        k = 0;
                        for (; k < size; ++k) {
                            auto rhs = el.markers[k];
                            if (s == k) continue;
                            if (rhs.kind == MarkerKind::Insert) continue;

                            //   |--------- Token ----------|
                            //      |---- span 1-------|
                            //         |--span 2---|
                            //      |-3|-- span 4--||-- span 5--|--6--|
                            if (lhs.span.intersects(rhs.span) && lhs.span.start() < rhs.span.start()) {
                                found = true;
                                break;
                            }
                        }
                        if (found) break;
                    }

                    if (!found) break;
                    assert(s != k);
                    auto lhs = el.markers[s];
                    auto rhs = el.markers[k];
                    auto l0 = Span(lhs.span.start(), rhs.span.start());
                    auto l1 = Span(rhs.span.start(), lhs.span.end());
                    el.markers[s].span = l0;
                    lhs.span = l1;
                    lhs.is_start = false;
                    // TODO: improve this by find the lower bound and insert after that position.
                    el.markers.push_back(lhs);
                    std::stable_sort(el.markers.begin(), el.markers.end(), cmp_op);
                }

                // split marked and non-marked text

                // for (auto const& item: el.markers) {
                //     std::println("[{}, {}, {}], ", item.annotation_index, item.span, item.is_start);
                // }

                // make the spans relative
                for (auto& m: el.markers) {
                    m.span = m.span - el.token_start_offset;
                }
            }

            // isolate marked tokens
            auto tokens = std::move(l.tokens);
            for (auto t = 0ul; t < tokens.size(); ++t) {
                auto& token = tokens[t];
                if (token.markers.empty()) {
                    l.tokens.push_back(std::move(token));
                    continue;
                }

                auto start_index = std::size_t{};
                auto end_index = start_index + 1;

                auto text = token.text.to_borrowed();
                auto previous_start = std::size_t{};

                while (start_index < token.markers.size()) {
                    auto start = token.markers[start_index].span.start();
                    auto end  = token.markers[start_index].span.end();

                    for (; end_index < token.markers.size(); ++end_index) {
                        auto const& m = token.markers[end_index];
                        if (m.span.start() != start) {
                            break;
                        }
                        end = std::max(end, m.span.end());
                    }

                    // Whole token text is marked.
                    if (text.size() == end - start) {
                        l.tokens.push_back(std::move(token));
                        previous_start = text.size();
                        break;
                    }

                    // Assumption: Spans are not empty.
                    assert(end != start && "Span should not be empty.");

                    auto markers = core::SmallVec<DiagnosticMarker, 2>{};
                    for (auto i = start_index; i < end_index; ++i) {
                        auto m = token.markers[i];
                        m.span = Span::from_size(m.span.start() - start, m.span.size());
                        markers.push_back(std::move(m));
                    }

                    if (start != previous_start) {
                        auto size = (start - previous_start);
                        l.tokens.push_back(NormalizedDiagnosticTokenInfo {
                            .text = token.text.substr(previous_start, size),
                            .markers = {},
                            .token_start_offset = token.token_start_offset,
                            .text_color = token.text_color,
                            .bg_color = token.bg_color,
                            .bold = token.bold,
                            .italic = token.italic
                        });
                        token.token_start_offset += size;
                    }

                    {
                        auto size = (end - start);
                        l.tokens.push_back(NormalizedDiagnosticTokenInfo {
                            .text = token.text.substr(start, size),
                            .markers = std::move(markers),
                            .token_start_offset = token.token_start_offset,
                            .text_color = token.text_color,
                            .bg_color = token.bg_color,
                            .bold = token.bold,
                            .italic = token.italic
                        });
                        token.token_start_offset += size;
                    }

                    previous_start = end;
                    start_index = end_index;
                }

                if (text.size() > previous_start) {
                    l.tokens.push_back(NormalizedDiagnosticTokenInfo {
                        .text = token.text.substr(previous_start),
                        .markers = {},
                        .token_start_offset = token.token_start_offset,
                        .text_color = token.text_color,
                        .bg_color = token.bg_color,
                        .bold = token.bold,
                        .italic = token.italic
                    });
                }
            }
            // for (auto t = 0ul; t < l.tokens.size(); ++t) {
            //     auto& token = l.tokens[t];
            //     std::println("HERE: '{}' | {}\n", token.text.to_borrowed(), token.markers.size());
            // }
        }

        return res;
    }

    static inline auto fix_newlines(
        core::SmallVec<DiagnosticLineTokens>& lines
    ) noexcept -> void {
        for (auto l = 0ul; l < lines.size(); ++l) {
            // Find the token text with newline
            auto& line = lines[l];
            auto i = 0ul;
            for (; i < line.tokens.size(); ++i) {
                auto text = line.tokens[i].text.to_borrowed();
                if (text.contains('\n')) break;
            }
            // continue the loop if no newline found.
            if (i >= line.tokens.size()) continue;

            auto& text = line.tokens[i].text;
            auto pos = text.to_borrowed().find('\n');
            DiagnosticLineTokens nl{
                .tokens = {},
                .line_number = line.line_number + 1,
                .line_start_offset = static_cast<dsize_t>(line.line_start_offset + pos)
            };
            auto marker = line.tokens[i].marker;
            auto offset = static_cast<dsize_t>(pos);

            DiagnosticTokenInfo token{
                .text = text.substr(pos + 1),
                .token_start_offset = line.tokens[i].token_start_offset + offset,
                .marker = Span(
                    std::max(marker.start(), line.tokens[i].token_start_offset + offset + 1),
                    marker.end()
                ),
                .text_color = line.tokens[i].text_color,
                .bg_color = line.tokens[i].bg_color,
                .bold = line.tokens[i].bold,
                .italic = line.tokens[i].italic,
            };
            if (token.marker.empty()) token.marker = {};
            nl.tokens.push_back(std::move(token));

            for (auto j = i + 1; j < line.tokens.size(); ++j) {
                nl.tokens.push_back(std::move(line.tokens[j]));
            }

            line.tokens[i].text = text.substr(0, pos);
            line.tokens[i].marker = Span(
                marker.start(),
                std::min(marker.end(), line.tokens[i].span().start() + offset)
            );
            lines.push_back(std::move(nl));
        }

        std::stable_sort(lines.begin(), lines.end(), [](DiagnosticLineTokens const& lhs, DiagnosticLineTokens const& rhs) {
            return lhs.line_start_offset < rhs.line_start_offset;
        });
    }

    // Message index to marker coords
    using message_marker_t = std::unordered_map<term::Point, core::SmallVec<DiagnosticMarker, 2>>;

    static inline auto render_source_text(
        term::Canvas& canvas,
        Diagnostic& diag,
        NormalizedDiagnosticAnnotations& as,
        term::BoundingBox ruler_container,
        term::BoundingBox container,
        message_marker_t& marker_to_message,
        DiagnosticRenderConfig const& config
    ) noexcept -> term::BoundingBox {
        static constexpr auto tab_width = term::Canvas::tab_width;
        char tab_indent_buff[tab_width] = {' '};
        std::fill_n(tab_indent_buff, tab_width, ' ');
        auto tab_indent = std::string_view(tab_indent_buff, tab_width);
        static_assert(tab_width > 0);

        auto& lines = diag.location.source.lines;
        fix_newlines(lines);
        auto x = container.x;

        auto skip_check_for = 0ul;
        // Point -> annotation index

        auto has_any_marker = [&as](DiagnosticLineTokens const& line) {
            if (line.has_any_marker()) return true;
            for (auto const& el: as.spans) {
                if (line.span().intersects(el.span)) {
                    return true;
                }
            }
            return false;
        };

        auto draw_ruler_helper = [&config, &canvas](
            term::BoundingBox& container,
            unsigned y,
            std::optional<unsigned> line_number = {}
        ) {
            if (container.y >= y) return;
            container.y += 1;
            while (container.y < y) {
                render_ruler(
                    canvas,
                    container,
                    line_number,
                    "",
                    config.dotted_vertical,
                    config.ruler_color
                );
                container.y++;
            }
        };

        ruler_container.y = container.y;

        for (auto l = 0ul; l < lines.size();) {
            auto& line = lines[l];

            // Adds ellipsis if consecutive non-marked lines are present, and it exceeds `max_non_marker_lines` from config
            if (!has_any_marker(line) && skip_check_for == 0) {
                auto number_of_non_marker_lines = 0ul;
                auto old_l = l;

                while (l < lines.size() && !has_any_marker(lines[l])) {
                    // calculate line start so we can use it for start of "..."
                    ++l;
                    ++number_of_non_marker_lines;
                }

                if (number_of_non_marker_lines >= config.max_non_marker_lines) {
                    ruler_container = render_ruler(
                        canvas,
                        ruler_container,
                        {},
                        "",
                        config.dotted_vertical,
                        config.ruler_color
                    );
                    canvas.draw_text(
                        std::format("... skipped {} lines ...", number_of_non_marker_lines),
                        container.x + 4,
                        container.y,
                        { .dim = true, .italic = true, .group_id = GroupId::diagnostic_source }
                    );
                    container.y++;
                    continue;
                }

                skip_check_for = l - old_l;
                l = old_l;
            } else {
                skip_check_for = std::max(skip_check_for, 1ul) - 1;
            }

            draw_ruler_helper(ruler_container, container.y);
            render_ruler(
                canvas,
                ruler_container,
                line.line_number,
                "",
                config.dotted_vertical,
                config.ruler_color
            );

            ++l;
            if (line.tokens.empty()) continue;

            auto normalized_tokens = normalize_diagnostic_line(line, as);

            std::size_t const total_canvas_cols = container.width;

            auto try_render_line = [total_canvas_cols](
                std::span<NormalizedDiagnosticTokenInfo> const& tokens
            ) -> std::pair<bool, std::size_t> {
                auto cols_occupied = std::size_t{};
                for (auto token_index = 0ul; token_index < tokens.size(); ++token_index) {
                    auto const& token = tokens[token_index];
                    auto text = token.text.to_borrowed();
                    if (text.empty()) continue;

                    auto overflow_span = token.overflow_span;

                    if (token.overflow != term::TextOverflow::none) {
                        cols_occupied += 3;
                    }

                    for (auto text_index = 0ul; text_index < overflow_span.start();) {
                        auto len = core::utf8::get_length(text[text_index]);
                        assert(text.size() >= text_index + len);
                        auto idx = text_index;
                        text_index += len;

                        auto c = text[idx];
                        cols_occupied += c == '\t' ? tab_width : 1u;
                    }

                    for (auto text_index = overflow_span.end(); text_index < text.size();) {
                        auto len = core::utf8::get_length(text[text_index]);
                        assert(text.size() >= text_index + len);
                        auto idx = text_index;
                        text_index += len;

                        auto c = text[idx];
                        cols_occupied += c == '\t' ? tab_width : 1u;
                    }
                }

                return { cols_occupied < total_canvas_cols, cols_occupied };
            };

            x = container.x;

            unsigned failed_count{0};
            // use this container as stack so we can insert new tokens while iterating
            std::reverse(normalized_tokens.begin(), normalized_tokens.end());
            bool is_first_line{true};
            while (!normalized_tokens.empty()) {
                auto line_of_tokens = std::move(normalized_tokens.back());
                normalized_tokens.pop_back();

                render_ruler(
                    canvas,
                    ruler_container,
                    line_of_tokens.line_number == 0 ? std::optional<dsize_t>{} : std::optional<dsize_t>{line_of_tokens.line_number},
                    "",
                    config.dotted_vertical,
                    config.ruler_color
                );

                bool success = false;
                do {
                    x = container.x;
                    auto result = try_render_line(line_of_tokens.tokens);
                    success = result.first;
                    auto cols_occupied = result.second;
                    failed_count += static_cast<unsigned>(success);

                    if (success) {
                        failed_count = 0;
                        // Case 1: Everything fits in one line
                        //  Ex: 1
                        //    | void main(int argc, char** argv) |
                        //    |           ~~~                    |
                        //  Ex: 2
                        //    | void main(int argc, char** argv) |
                        //    |           ~~~       ~~~~~~       |
                        // Case 2: Does not fit because start has too much space
                        //  Ex: 1
                        //    |                 void main(int arg|
                        //    | c, char** argv)                  |
                        //  Sol: Remove space until it fits the screen
                        //
                        //  Ex: 2
                        //    |                 void main(int arg|
                        //    | c, char** argv) { return 0; }    |
                        //  Fixed:
                        //    | void main(int argc, char** arg...|
                        //    |           ~~~       ~~~~~~       |
                        //
                        //  Ex: 3
                        //    |                 void main(int arg|
                        //    | c, char** argv) { return 0; }    |
                        //  Fixed:
                        //    | void main(int argc, char** argv) |
                        //    |           ~~~       ~~~~~~       |
                        //    |
                        //    | { return 0; }                    |
                        //    |   ~~~~~~                         |

                        auto free_space = total_canvas_cols - std::min(total_canvas_cols, cols_occupied);
                        auto start_padding = std::min<std::size_t>(
                            std::max(free_space, std::size_t{1}) - 1,
                            line_of_tokens.tokens[0].token_start_offset - line_of_tokens.line_start_offset
                        );
                        x += static_cast<unsigned>(start_padding * is_first_line);
                        auto bottom_padding = 0u;
                        unsigned message_bottom_padding{0};
                        for (auto const& token: line_of_tokens.tokens) {
                            auto token_x_pos = x;

                            auto text = token.text.to_borrowed();
                            if (text.empty()) continue;

                            auto marker_start = text.size();
                            if (!token.markers.empty()) {
                                marker_start = token.markers[0].span.start();
                            }

                            auto render_text = [
                                text,
                                &canvas,
                                tab_indent,
                                overflow = token.overflow,
                                overflow_span = token.overflow_span
                            ](
                                std::size_t start,
                                std::size_t end,
                                unsigned x,
                                unsigned y,
                                term::Style style
                            ) -> unsigned {
                                if (end - start == 0) return x;
                                if (overflow == term::TextOverflow::start_ellipsis) {
                                    auto tmp_style = style;
                                    tmp_style.bold = true;
                                    tmp_style.dim = true;
                                    for (auto i = 0ul; i < 3; ++i, ++x) {
                                        canvas.draw_pixel(x, y, ".", tmp_style);
                                    }
                                }

                                style.group_id = GroupId::diagnostic_source;

                                auto body = [&x, y, &canvas, style, tab_indent](std::string_view txt) {
                                    if (txt == "\t") {
                                        canvas.draw_pixel(
                                            x, y,
                                            tab_indent,
                                            style
                                        );
                                        x += (tab_indent.size() - 1);
                                    } else {
                                        canvas.draw_pixel(
                                            x, y,
                                            txt,
                                            style
                                        );
                                    }
                                };

                                for (auto i = start; i < overflow_span.start(); ++x) {
                                    auto len = core::utf8::get_length(text[i]);
                                    assert(text.size() >= i + len);
                                    auto txt = text.substr(i, len);
                                    body(txt);
                                    i += len;
                                }

                                if (overflow == term::TextOverflow::middle_ellipsis) {
                                    auto tmp_style = style;
                                    tmp_style.bold = true;
                                    tmp_style.dim = true;
                                    for (auto j = 0ul; j < 3; ++j, ++x) {
                                        canvas.draw_pixel(x, y, ".", tmp_style);
                                    }
                                }

                                for (auto i = std::max<std::size_t>(overflow_span.end(), start); i < end; ++x) {
                                    auto len = core::utf8::get_length(text[i]);
                                    assert(text.size() >= i + len);
                                    auto txt = text.substr(i, len);
                                    body(txt);
                                    i += len;
                                }

                                if (overflow == term::TextOverflow::ellipsis) {
                                    style.bold = true;
                                    style.dim = true;
                                    for (auto i = 0ul; i < 3; ++i, ++x) {
                                        canvas.draw_pixel(x, y, ".", style);
                                    }
                                }
                                return x;
                            };

                            // Render before marker
                            x = render_text(0, marker_start, x, container.y, token.to_style());

                            auto marker_end = marker_start;
                            if (!token.markers.empty()) {
                                // Positions of marker:
                                // Primary Marker > Error > Warning >  
                                std::array<std::pair<unsigned, unsigned>, diagnostic_level_elements_count> marker_freq{};
                                std::fill(marker_freq.begin(), marker_freq.end(), std::make_pair(0, std::numeric_limits<unsigned>::max()));
                                unsigned marker_rel_pos{};

                                marker_start = token.markers[0].span.start();
                                // 1. precompute markers and set non-secondary markers' relative position to 0.
                                {
                                    auto primary_span = Span();
                                    for (auto const& m: token.markers) {
                                        if (m.kind == MarkerKind::Primary) {
                                            primary_span = m.span;
                                            break;
                                        }
                                    }

                                    for (auto const& m: token.markers) {
                                        marker_end = std::max<std::size_t>(marker_end, m.span.end());

                                        DiagnosticLevel level;
                                        if (m.kind == MarkerKind::Primary) {
                                            level = diag.level;
                                        } else {
                                            auto a_index = m.annotation_index;
                                            level = as.spans[a_index].level;
                                        }
                                        auto& [freq, rel_pos] = marker_freq[static_cast<unsigned>(level)];
                                        ++freq;
                                        if (m.kind != MarkerKind::Secondary || primary_span == m.span) {
                                            rel_pos = 0;
                                            marker_rel_pos = 1;
                                        }
                                    }
                                }

                                // Case 3: Overlapping markers
                                //  Ex:
                                //    | let xy = func(a, b);           |
                                //    |       ~~~~~~~~                 |
                                //    |       ~~~~~~~~~~~~~            |
                                //    |       ~~~~~~~                  |
                                // INFO:: Segment tree could be used here if the current implementation
                                // has performance some implications.
                                core::SmallVec<std::array<unsigned, diagnostic_level_elements_count>, 64> marker_count_for_each_cell(marker_end + 1, {});

                                // increment the markers in each cells
                                for (auto const& m: token.markers) {
                                    DiagnosticLevel level;
                                    if (m.kind == MarkerKind::Primary) {
                                        level = diag.level;
                                    } else {
                                        auto a_index = m.annotation_index;
                                        level = as.spans[a_index].level;
                                    }
                                    for (auto s = m.span.start(); s < m.span.end(); ++s) {
                                        marker_count_for_each_cell[s][static_cast<unsigned>(level)]++;
                                    }
                                }

                                // Higher the enum value has more priority.
                                for (auto it = marker_freq.rbegin(); it != marker_freq.rend(); ++it) {
                                    auto& [freq, rel_pos] = *it;
                                    if (rel_pos != std::numeric_limits<unsigned>::max()) continue;
                                    if (freq == 0) continue;
                                    rel_pos = marker_rel_pos++;
                                }

                                auto max_pt = term::Point(
                                    token_x_pos,
                                    container.y
                                );

                                for (auto const& m: token.markers) {
                                    DiagnosticLevel d_level;
                                    if (m.kind == MarkerKind::Primary) {
                                        d_level = diag.level;
                                    } else {
                                        auto a_index = m.annotation_index;
                                        d_level = as.spans[a_index].level;
                                    }
                                    [[maybe_unused]] auto [freq, rel_y_pos] = marker_freq[static_cast<std::size_t>(d_level)];

                                    max_pt.x = std::max(token_x_pos + m.span.start(), max_pt.x);
                                    max_pt.y = std::max(container.y + 1 + rel_y_pos, max_pt.y);
                                }

                                // 2. store the span and render marker
                                for (auto const& m: token.markers) {
                                    DiagnosticLevel d_level;
                                    if (m.kind == MarkerKind::Primary) {
                                        d_level = diag.level;
                                    } else {
                                        auto a_index = m.annotation_index;
                                        d_level = as.spans[a_index].level;
                                    }
                                    auto [freq, rel_y_pos] = marker_freq[static_cast<std::size_t>(d_level)];
                                    auto pt = term::Point(
                                        token_x_pos + m.span.start(),
                                        container.y + 1u + rel_y_pos
                                    );
                                    if (m.is_start && m.kind != MarkerKind::Primary) {
                                        auto& marker_to_message_item = marker_to_message[max_pt];
                                        if (as.spans[m.annotation_index].message_index != DiagnosticMessageSpanInfo::npos) {
                                            message_bottom_padding = 1;
                                        }
                                        marker_to_message_item.push_back(m);
                                    }

                                    auto tx = pt.x;
                                    // render each marker with token text
                                    for (auto s = m.span.start(); s < m.span.end();) {
                                        auto pos = s;
                                        auto len = core::utf8::get_length(text[pos]);
                                        s += len;
                                        assert(pos + len <= text.size() && "invalid utf-8 character");
                                        auto txt = text.substr(pos, len);

                                        auto marker = std::string_view{};
                                        int z_index = static_cast<int>(d_level);
                                        auto color = diagnostic_level_to_color(std::span(config.level_to_color), d_level);
                                        auto style = token.to_style();

                                        switch (m.kind) {
                                            case MarkerKind::Primary: {
                                                marker = config.markers.primary;
                                                z_index += 50;
                                            } break;
                                            case MarkerKind::Secondary: {
                                                auto count = marker_count_for_each_cell[pos][static_cast<unsigned>(d_level)];
                                                switch (count) {
                                                    case 0: case 1: marker = config.markers.single; break;
                                                    case 2: marker = config.markers.double_; break;
                                                    case 3: marker = config.markers.tripple; break;
                                                    default: marker = config.markers.quad; break;
                                                }
                                            } break;
                                            case MarkerKind::Insert: {
                                                marker = config.markers.insert;
                                                z_index += 50;
                                                style.text_color = color;
                                                style.bold = true;
                                            } break;
                                            case MarkerKind::Delete: {
                                                marker = config.markers.remove; 
                                                z_index += 50;
                                                style.strike = true;
                                                style.dim = true;
                                                style.text_color = color;
                                            } break;
                                        }

                                        auto iter = 1u;
                                        auto tmp_text = txt;
                                        if (txt[0] == '\t') {
                                            tmp_text = tab_indent;
                                            iter = tab_width;
                                        }

                                        style.z_index = z_index;
                                        style.bold = true;
                                        if (style.text_color == Color::Default) {
                                            style.text_color = color;
                                        }
                                        style.group_id = GroupId::diagnostic_source;
                                        for (auto k = 0ul; k < iter; ++k, ++tx) {
                                            canvas.draw_pixel(
                                                tx,
                                                container.y,
                                                tmp_text,
                                                style
                                            );
                                            canvas.draw_pixel(
                                                tx,
                                                pt.y,
                                                marker,
                                                {
                                                    .text_color = color,
                                                    .bold = true,
                                                    .z_index = z_index
                                                }
                                            );
                                        }

                                        auto rc = ruler_container;
                                        rc.y = pt.y;
                                        render_ruler(
                                            canvas,
                                            rc,
                                            {},
                                            "",
                                            config.dotted_vertical,
                                            config.ruler_color
                                        );
                                    }
                                    x = std::max(tx, x);
                                }

                                bottom_padding = std::max(marker_rel_pos + 1, bottom_padding);
                            }

                            // Render after the marker
                            x = render_text(marker_end, text.size(), x, container.y, token.to_style());
                        }
                        bottom_padding += message_bottom_padding;
                        container.y += bottom_padding;
                        draw_ruler_helper(ruler_container, container.y + 1);
                        break;
                    }

                    // try to remove non-essential parts.
                    // If there is nothing to remove, then move some of it to
                    // next line.


                    // Case 4: Highlight spans token that breaks across lines
                    //  Ex: 1
                    //    | fn run(config: VeryLongConfigTyp |
                    //    | eThatBreaksLines)                |
                    //    |       ~~~~~~~~~~~~~~~~~~~~~~~~~~ |
                    //  Ex: 2
                    //    | fn run(config: VeryLongConfigTyp |
                    //    |       ~~~~~~~~~~~~~~~~~~~~~~~~~~ |
                    //    | eThatBreaksLines)                |
                    //    | ~~~~~~~~~~~~~~~~~                |

                    // Case 5: Multiple markers with large space in between
                    //  Ex: 1
                    //    | fn connect(src: &str, ..., dst: &str) |
                    //    |             ~~~~           ~~~~       |

                    // Algorithm:
                    // 1. calculate center of mass.
                    // 2. calulate marker relative to center of mass.
                    // 3. start removing from largest distance.
                    // Formula: (sum(i) m_i * x_i)/(sum(i) m_i); where x_i is the center of the span
                    // and m_i is differs based on marker kind. Primary > Insert = Delete > Secondary
                    //
                    // Ex 1. Left side has less distance than the right side.
                    // BEFORE: |xxxxxxxxxxxx COM xxxxxxxxxxxxxxxxxxxxxxxxxxxxx|
                    // AFTER:  |xxxxxxxxxxxx COM xxxxxxxxxxxx|
                    //
                    // Ex 2. Right side has less distance than the left side.
                    // BEFORE: |xxxxxxxxxxxxxxxxxxxxxxxxxxxxx COM xxxxxxxxxxxx|
                    // AFTER:  |xxxxxxxxxxxx COM xxxxxxxxxxxx|

                    auto count_text_len = [](std::string_view text) -> std::size_t {
                        auto tab_count = static_cast<std::size_t>(std::count(text.begin(), text.end(), '\t'));
                        return core::utf8::calculate_size(text) + (tab_count * (tab_width - 1));
                    };

                    std::size_t com{};
                    std::size_t total_mass{};
                    auto offset = std::size_t{};
                    for (auto const& token: line_of_tokens.tokens) {
                        auto old_offset = offset;
                        auto text = token.text.to_borrowed();
                        offset += count_text_len(text) - token.overflow_span.size();
                        if (token.markers.empty()) {
                            continue;
                        }
                        for (auto const& m: token.markers) {
                            auto const center = m.span.center() + old_offset;
                            auto const mass = mass_from_marker(m.kind);
                            total_mass += mass;
                            com += mass * center;
                        }
                    }
                    com /= std::max(total_mass, std::size_t{1});

                    // Case 1: Left Side
                    //   P: [xxxxxxxxxxx][xxxx][xxxxxxxxx][~~~~~~~]
                    //   S: ...[xxxx][xxxxxxxxx][~~~~~~~]
                    // Case 2: Right Side
                    //   P: [~~~~~~~][xxxxxxxxxxx][xxxx][xxxxxxxxx]
                    //   S: [~~~~~~~][xxxxxxxxxxx][xxxx]...

                    auto trim_end_of_text = [
                        &cols_occupied,
                        total_canvas_cols,
                        &line_of_tokens,
                        count_text_len
                    ]<typename I>(I start, I end, bool is_reverse) -> I {
                        if (cols_occupied < total_canvas_cols) return end;

                        auto it = start;
                        cols_occupied += 3;
                        for (; it != end; ++it) {
                            auto diff = cols_occupied - total_canvas_cols + 1;

                            NormalizedDiagnosticTokenInfo& token = *it;
                            auto txt = token.text.to_borrowed();
                            if (!token.markers.empty()) {
                                break;
                            }
                            auto text_size = count_text_len(token.text.to_borrowed());
                            if (text_size < diff) {
                                auto is_next_marker{false};
                                if (it + 1 < end) {
                                    is_next_marker = !(it + 1)->markers.empty();
                                }
                                if (text_size > 3 || !is_next_marker) {
                                    cols_occupied -= text_size;
                                }
                                token.overflow = term::TextOverflow::none;
                                token.overflow_span = {};

                                if (is_next_marker && !txt.empty()) {
                                    if (is_reverse) {
                                        token.overflow = term::TextOverflow::ellipsis;
                                    } else {
                                        token.overflow = term::TextOverflow::start_ellipsis;
                                    }
                                    token.overflow_span = Span(0, static_cast<dsize_t>(txt.size()));
                                    break;
                                }
                            } else {
                                if (is_reverse) {
                                    auto s = std::size_t{};
                                    while(s < txt.size()) {
                                        auto len = core::utf8::get_length(txt[s]);
                                        assert(s + len <= txt.size());
                                        auto dec = (txt[s] == '\t' ? tab_width : 1u);
                                        if (total_canvas_cols + text_size <= cols_occupied + dec) {
                                            cols_occupied -= text_size;
                                            break;
                                        }

                                        text_size -= dec;
                                        s += len;
                                    }
                                    token.overflow_span = Span(
                                        static_cast<dsize_t>(s),
                                        static_cast<dsize_t>(txt.size())
                                    );
                                    token.overflow = term::TextOverflow::ellipsis;
                                } else {
                                    auto s = std::size_t{};
                                    while(s < txt.size() && total_canvas_cols <= cols_occupied) {
                                        auto len = core::utf8::get_length(txt[s]);
                                        assert(s + len <= txt.size());
                                        cols_occupied -= txt[s] == '\t' ? tab_width : 1u;
                                        s += len;
                                    }
                                    token.overflow_span = Span::from_size(
                                        0,
                                        static_cast<dsize_t>(s)
                                    );
                                    token.overflow = term::TextOverflow::start_ellipsis;
                                }
                            }
                            if (cols_occupied < total_canvas_cols) break;
                        }

                        if (it - start > 0) {
                            auto sz = it - start;
                            if (is_reverse) {
                                for (auto i = 0; i < sz; ++i) {
                                    line_of_tokens.tokens.pop_back();
                                }
                            } else {
                                line_of_tokens.tokens.erase(line_of_tokens.tokens.begin(), line_of_tokens.tokens.begin() + sz);
                            }
                        }
                        return it;
                    };

                    // Case 2: Between markers
                    //   P: [~~~~~][xxxxxxxxxxx][xxxx][xxxxxxxxx][~~~~~~~]
                    //   S: [~~~~~][xxxxxxxx]...[xxxx][xxxxxx][~~~~~~~]
                    //
                    //   P: [~~~~~][xxxxxxxxxxx][~~~~~~~]
                    //   S: [~~~~~][xxx]...[xxx][~~~~~~~]
                    auto trim_between = [
                        &cols_occupied,
                        total_canvas_cols,
                        count_text_len
                    ]<typename I>(I start, I end) {
                        cols_occupied += 3;
                        do {
                            // Find the first marker
                            while (start != end) {
                                if (!start->markers.empty()) break;
                                ++start;
                            }
                            if (start == end) break;

                            auto first_marker = start;
                            auto second_marker = start + 1;
                            // Find the second marker after the first one.
                            for (; second_marker != end; ++second_marker) {
                                if (!second_marker->markers.empty()) {
                                    break;
                                }
                            }
                            auto distance = second_marker - first_marker;
                            // If there are no markers between the markers, start collapsing
                            if (distance > 1) {
                                auto diff = cols_occupied - total_canvas_cols + 1;
                                auto b = first_marker + 1;
                                auto e = second_marker;
                                std::size_t actual_size{};
                                for (auto it = b; it != e; ++it) {
                                    NormalizedDiagnosticTokenInfo& token = *it;
                                    actual_size += count_text_len(token.text.to_borrowed());
                                }
                                if (actual_size <= diff) {
                                    {
                                        NormalizedDiagnosticTokenInfo& token = *b;
                                        token.overflow_span = Span(0, static_cast<dsize_t>(token.text.size()));
                                        token.overflow = term::TextOverflow::middle_ellipsis;
                                    }

                                    for (auto it = b + 1; it != end; ++it) {
                                        NormalizedDiagnosticTokenInfo& token = *it;
                                        token.overflow_span = Span(0, static_cast<dsize_t>(token.text.size()));
                                        token.overflow = term::TextOverflow::none;
                                    }
                                    cols_occupied -= actual_size;
                                } else {
                                    auto mid = actual_size / 2;
                                    auto left_size = diff / 2;
                                    auto low = mid - left_size;

                                    auto it = b;
                                    for (; it != e; ++it) {
                                        NormalizedDiagnosticTokenInfo& token = *it;
                                        auto txt = token.text.to_borrowed();
                                        auto i = std::size_t{};
                                        auto found{false};
                                        for (; i < txt.size();) {
                                            auto len = core::utf8::get_length(txt[i]);
                                            assert(i + len <= txt.size());
                                            auto inc = txt[i] == '\t' ? tab_width : 1;
                                            if (low >= inc) {
                                                low -= inc;
                                            } else {
                                                found = true;
                                                break;
                                            }
                                            i += len;
                                        }

                                        if (found) { 
                                            std::size_t ed = i;
                                            while (ed < txt.size() && diff > 0) {
                                                auto len = core::utf8::get_length(txt[ed]);
                                                assert(ed + len <= txt.size());
                                                auto inc = txt[ed] == '\t' ? tab_width : 1;
                                                diff -= std::min(diff, inc);
                                                ed += len;
                                            }

                                            token.overflow_span = Span(
                                                static_cast<dsize_t>(i),
                                                static_cast<dsize_t>(std::min(txt.size(), ed))
                                            );
                                            token.overflow = term::TextOverflow::middle_ellipsis;
                                            diff -= std::min(diff, count_text_len(txt.substr(i)));
                                            if (diff == 0) return;
                                            break;
                                        }
                                    }

                                    for (; it != e; ++it) {
                                        NormalizedDiagnosticTokenInfo& token = *it;
                                        auto txt = token.text.to_borrowed();
                                        auto i = std::size_t{};
                                        while (i < txt.size() && diff > 0) {
                                            auto len = core::utf8::get_length(txt[i]);
                                            assert(i + len <= txt.size());
                                            auto inc = txt[i] == '\t' ? tab_width : 1;

                                            diff -= std::min(diff, inc);

                                            if (diff == 0) {
                                                token.overflow_span = Span(
                                                    static_cast<dsize_t>(0),
                                                    static_cast<dsize_t>(i + len)
                                                );
                                                token.overflow = term::TextOverflow::none;
                                            }
                                            i += len;
                                        }
                                    }
                                }
                            }

                            start = second_marker;
                        } while (start != end && cols_occupied >= total_canvas_cols);
                    };

                    // auto old_occupied_cols = cols_occupied;

                    // three pass trim:
                    // Pass 1: go from left/right to end.
                    // Pass 2: go from the oposite side to end.
                    // Pass 3: collapse between markers.
                    if (offset > com * 2) {
                        // left is smaller
                        // BEFORE: |xxxxxxxxxxxx COM xxxxxxxxxxxxxxxxxxxxxxxxxxxxx|
                        auto it = trim_end_of_text(line_of_tokens.tokens.rbegin(), line_of_tokens.tokens.rend(), true);

                        // Collapse from the other end
                        if (cols_occupied >= total_canvas_cols) {
                            auto diff = it - line_of_tokens.tokens.rbegin();
                            auto end = line_of_tokens.tokens.end() - diff;
                            trim_end_of_text(line_of_tokens.tokens.begin(), end, false);
                        }

                        // Collapse between the marker
                        if (cols_occupied >= total_canvas_cols) {
                            trim_between(it, line_of_tokens.tokens.rend());
                        }

                    } else {
                        // right is smaller
                        // BEFORE: |xxxxxxxxxxxxxxxxxxxxxxxxxxxxx COM xxxxxxxxxxxx|
                        auto it = trim_end_of_text(line_of_tokens.tokens.begin(), line_of_tokens.tokens.end(), false);

                        // Collapse from the other end
                        if (cols_occupied >= total_canvas_cols) {
                            auto diff = it - line_of_tokens.tokens.begin();
                            auto end = line_of_tokens.tokens.rend() - diff;
                            trim_end_of_text(line_of_tokens.tokens.rbegin(), end, true);
                        }

                        // Collapse between the marker
                        if (cols_occupied >= total_canvas_cols) {
                            trim_between(it, line_of_tokens.tokens.end());
                        }
                    }
                    // If we aren't able to collapse anymore, we split it into lines
                    if (failed_count > 1) {
                        failed_count = 0;
                        auto i = 0ul;
                        auto occupied = std::size_t{};
                        for (; i < line_of_tokens.tokens.size(); ++i) {
                            auto& token = line_of_tokens.tokens[i];
                            auto cols = count_text_len(token.text.to_borrowed());
                            if (occupied + cols) {
                                break;
                            }
                            occupied += cols;
                        }

                        if (i < line_of_tokens.tokens.size()) {
                            NormalizedDiagnosticLineTokens tmp{
                                .tokens = {},
                                .line_number = {},
                                .line_start_offset = line_of_tokens.line_start_offset
                            };

                            auto& token = line_of_tokens.tokens[i];
                            auto text = token.text.to_borrowed();
                            for (auto j = 0ul; j < text.size();) {
                                auto len = core::utf8::get_length(text[j]);
                                auto inc = text[j] == '\t' ? tab_width : 1u;
                                if (occupied + inc >= total_canvas_cols) {
                                    auto markers = token.markers;
                                    token.markers.clear();
                                    auto sz = static_cast<dsize_t>(j);
                                    NormalizedDiagnosticTokenInfo t0{
                                        .text = token.text.substr(j),
                                        .markers = {},
                                        .token_start_offset = 0,
                                        .text_color = token.text_color,
                                        .bg_color = token.bg_color,
                                        .bold = token.bold,
                                        .italic = token.italic
                                    };
                                    for (auto& m: markers) {
                                        auto left = m;
                                        auto right = m;
                                        left.span = Span(
                                            m.span.start(),
                                            std::min(sz, m.span.end())
                                        );
                                        right.span = Span(
                                            std::max(m.span.start(), sz) - sz,
                                            m.span.end()
                                        );
                                        if (!left.span.empty()) {
                                            token.markers.push_back(left);
                                            right.is_start = false;
                                        }
                                        if (!right.span.empty()) t0.markers.push_back(right);
                                    }
                                    token.text = token.text.substr(0, j);
                                    tmp.line_start_offset += sz;
                                    tmp.tokens.push_back(std::move(t0));
                                    break;
                                }
                                occupied += inc;
                                j += len;
                            }

                            for (auto j = i + 1; j < line_of_tokens.tokens.size(); ++j) {
                                auto& tk = line_of_tokens.tokens[j];
                                tk.token_start_offset = 0;
                                tmp.tokens.push_back(std::move(tk));
                            }

                            normalized_tokens.push_back(std::move(tmp));
                        }
                    }
                    ++failed_count;
                } while (!success);

                if (!normalized_tokens.empty()) {
                    ++container.y;
                }
                is_first_line = false;
            }

            ++container.y;
        }

        return container;
    }

    // (marker position, message position)
    using point_container_t = core::SmallVec<std::tuple<
        term::Point /*Marker Position*/,
        term::BoundingBox /*Message Bounding Box*/,
        term::Style /*Marker style*/
    >>;

    static inline auto render_span_messages(
        term::Canvas& canvas,
        NormalizedDiagnosticAnnotations& as,
        term::BoundingBox ruler_container,
        term::BoundingBox container,
        message_marker_t const& message_markers,
        point_container_t& points,
        DiagnosticRenderConfig const& config
    ) -> term::BoundingBox {
        auto const container_center_x = container.top_left().first + container.width / 2;
        auto const min_x = container.bottom_left().first;
        auto const max_x = container.bottom_right().first;

        // 1. Messages can be grouped if they point to the same span.
        // 2. Spans can be grouped if they point to the same message.
        // 3. If a message is shared among different spans:
        //      case 1: if it's grouped with other messages, it's duplicated.
        //      case 2: it is not grouped, it's left alone.
        // 4. If a whole group is shared, single rendered message shared.
        // 5. If a subset of group is shared, that group is rendered separately.

        struct MessageInfo {
            using index_t = unsigned;
            index_t message_index;
            std::uint8_t level_bit_mask{}; // packed multiple diagnostic levels.
        };

        struct MessageGroup {
            core::SmallVec<MessageInfo, 1> messages;
            core::SmallVec<term::Point, 1> spans;
            unsigned x_pos{std::numeric_limits<unsigned>::max()};
        };

        core::SmallVec<MessageGroup> groups;

        using message_t = std::uint32_t;

        struct PackedMessageItem {
            std::uint16_t index;
            std::uint16_t level;

            constexpr auto to_int() const noexcept -> std::uint32_t { return std::bit_cast<message_t>(*this); }
            static constexpr auto from(std::uint32_t v) noexcept -> PackedMessageItem { return std::bit_cast<PackedMessageItem>(v); }
        };


        std::unordered_map<message_t, std::unordered_set<term::Point>> message_marker_sets{};

        auto total_points = std::size_t{};
        for (auto const& [pt, markers]: message_markers) {
            for (auto const& m: markers) {
                auto& tmp = as.spans[m.annotation_index];
                auto index = tmp.message_index;
                if (index == DiagnosticMessageSpanInfo::npos) continue;
                assert(index <= std::numeric_limits<std::uint16_t>::max());
                auto item = PackedMessageItem(
                    static_cast<std::uint16_t>(index),
                    static_cast<std::uint16_t>(tmp.level)
                ).to_int();
                message_marker_sets[item].insert(pt);
                ++total_points;
            }
        }

        static constexpr auto bit_masks = []{
            std::array<std::uint8_t, diagnostic_level_elements_count> res{};
            res[0] = 1;
            for (auto i = std::uint8_t{1}; i < diagnostic_level_elements_count; ++i) {
                res[i] = static_cast<std::uint8_t>(1 << i);
            }
            return res;
        }();


        {
            std::unordered_map<std::size_t /*message index*/, std::uint8_t /*level*/> message_level_map;

            core::SmallVec<std::uint32_t, 0> messages_to_be_removed; 
            message_level_map.reserve(as.messages.size());

            // keep matching sets with larger and larger size.
            for (auto sets = 1ul; sets <= total_points;) {
                auto it = message_marker_sets.begin();
                message_level_map.clear();
                messages_to_be_removed.clear();
                // find the set with size 'sets'
                for (; it != message_marker_sets.end(); ++it) {
                    if (it->second.size() == sets) break;
                }

                // if there is no set with size 'sets', move to next set size.
                if (it == message_marker_sets.end()) {
                    ++sets;
                    continue;
                }

                auto pi = PackedMessageItem::from(it->first);
                message_level_map[pi.index] |= bit_masks[pi.level];
                auto const& markers = it->second;
                messages_to_be_removed.push_back(it->first);

                ++it;

                // Find all the duplicate sets.
                for (; it != message_marker_sets.end(); ++it) {
                    if (it->second == markers) {
                        messages_to_be_removed.push_back(it->first);
                        pi = PackedMessageItem::from(it->first);
                        message_level_map[pi.index] |= bit_masks[pi.level];
                    }
                }

                auto g = MessageGroup{};
                for (auto [index, ls]: message_level_map) {
                    g.messages.push_back(MessageInfo{
                        .message_index = static_cast<MessageInfo::index_t>(index),
                        .level_bit_mask = ls,
                    });
                }

                auto avg_x = unsigned{};
                auto count = static_cast<unsigned>(markers.size());
                for (auto m: markers) {
                    g.spans.push_back(m);
                    avg_x += m.x;
                }

                // use the centroid/COM for the x coord
                g.x_pos = std::max(avg_x / std::max<unsigned>(1u, count), min_x);

                for (auto p: messages_to_be_removed) {
                    message_marker_sets.erase(p);
                }

                groups.push_back(std::move(g));
            }
        }

        // Sort the groups by the largest x position to the lowest.
        std::sort(groups.begin(), groups.end(), [](MessageGroup const& l, MessageGroup const& r) {
            return l.x_pos > r.x_pos;
        });

        {
            std::unordered_set<unsigned> x_positions;

            // shift the message left by 2 if they lies vertically below each other.
            for (auto& g: groups) {
                while (x_positions.contains(g.x_pos)) {
                    if (g.x_pos == min_x) break;
                    g.x_pos = std::max(g.x_pos - 2, g.x_pos);
                }

                x_positions.insert(g.x_pos);
            }
        }

        auto const character_limit = std::min(
            container.width - 6,
            static_cast<unsigned>(config.max_message_characters_per_line)
        );

        static constexpr auto shift_by = 2u;

        auto bit_iterator = [](std::size_t bits, auto&& fn) {
            while (bits) {
                auto index = static_cast<std::size_t>(std::countr_zero(bits));
                fn(index);

                // clear the right most set bit.
                bits &= bits - 1;
            }
        };

        auto last_box = term::BoundingBox{};
        auto max_y_rendered = container.y;

        for (auto t = 0ul; t < groups.size(); ++t) {
            auto const& g = groups[t];
            auto x_pos = g.x_pos;

            auto style = term::TextStyle {
                .word_wrap = true,
                .break_whitespace = true,
                .max_width = character_limit,
                .padding = term::PaddingValues(0, 2, 0, 2)
            };

            auto bp = config.bullet_point;
            bool should_show_bullet_points = (g.messages.size() > 1) && !bp.empty();

            auto measure_helper = [&](MessageInfo const& info) {
                auto diagnostic_count = std::size_t{};
                auto prefix_len = unsigned{};

                // iterator levels:
                // find number of diagnostics and max diagnostic name size
                {
                    bit_iterator(info.level_bit_mask, [&prefix_len, &diagnostic_count](std::size_t index) {
                        auto level = static_cast<DiagnosticLevel>(index);

                        prefix_len = std::max(
                            static_cast<unsigned>(to_string(level).size()) + /*'[' + ']'*/2,
                            prefix_len
                        );
                        ++diagnostic_count;
                    });
                }

                unsigned header_size = 0; // ─ Note ─
                if (diagnostic_count == 1) {
                    header_size = /*'─'*/ 1 + /*Diagnostic name*/ prefix_len + /*'─'*/ 1;
                    prefix_len = 0;
                }

                // take bullet points into account inside the x-coordinate.
                if (should_show_bullet_points) {
                    x_pos += core::utf8::calculate_size(bp) + /*padding for space*/ 1;
                }

                auto tmp_style = style;
                // to break even if it's not a whitespace since a whole
                // might not fit in a line if container is too small. So measure might
                // fail and return content width of zero.
                tmp_style.break_whitespace = false;

                return std::make_tuple(diagnostic_count, prefix_len, header_size, tmp_style);
            };

            // Now, try to find appropriate x coordinate of the message
            // so we can reduce the number of line breaks and squeezing the
            // message (avoids unreadable texts)
            if ((x_pos > container_center_x) || true) {
                for (auto const info: g.messages) {
                    auto const& message = as.messages[info.message_index];

                    auto [diagnostic_count, prefix_len, header_size, tmp_style] = measure_helper(info);

                    while (x_pos > container_center_x) {
                        auto box = canvas.measure_text(
                            message,
                            x_pos,
                            container.y,
                            tmp_style
                        );

                        box.width = std::max(box.width, header_size);

                        // calculate the upper end of the x-coordinate
                        // x = text end x position + padding for box + diagnostic name + padding after
                        //     diagnostic name + diagnostic indicator + place for bullet point.
                        auto pos = box.bottom_right().first + 4 + prefix_len + (diagnostic_count - 1) + (should_show_bullet_points ? 2 : 0);

                        // If we maximized the max characters, then we cannot do anything so we break.
                        if (style.max_width <= box.width + 2 && box.width != 0) break;

                        bool should_shift = pos >= container.bottom_right().first;
                        if (box.height > 2 || box.width == 0) should_shift = true;

                        if (should_shift) {
                            x_pos -= shift_by;
                        } else {
                            break;
                        }
                    }
                }
            }

            // Render messages
            {
                unsigned content_width{};
                if (last_box.width != 0) {
                    // Measure content width to find if the boxes are intersecting.
                    for (auto const info: g.messages) {
                        auto const& message = as.messages[info.message_index];

                        auto [diagnostic_count,
                              prefix_len,
                              header_size,
                              tmp_style
                             ] = measure_helper(info);

                        auto box = canvas.measure_text(
                            message,
                            x_pos,
                            container.y,
                            tmp_style
                        );

                        auto box_end = box.width + 5 + prefix_len + (diagnostic_count - 1) + (should_show_bullet_points ? 2 : 0);

                        content_width = std::max(content_width, static_cast<unsigned>(box_end));
                    }

                    auto content_box = term::BoundingBox{
                        .x = x_pos,
                        .y = last_box.y,
                        .width = content_width,
                        .height = last_box.height
                    };
                    // move the message down if the last message lies inside the current message bounding box.
                    if (last_box.intersects(content_box)) {
                        container.y += last_box.height;
                    }

                    content_width = 0;
                }

                container.y += 2;
                auto y = container.y + 1;
                // Diagnostic level that will be used for rendering boxes and paths.
                auto dominant_level = DiagnosticLevel::Help;

                for (auto const& info: g.messages) {
                    auto const& message = as.messages[info.message_index];
                    auto diagnostic_counts = std::size_t{};
                    auto current_level = DiagnosticLevel::Help;

                    // iterator levels:
                    {
                        bit_iterator(info.level_bit_mask, [&dominant_level, &current_level, &diagnostic_counts](std::size_t index) {
                            auto level = static_cast<DiagnosticLevel>(index);
                            dominant_level = std::max(level, dominant_level);
                            current_level = std::max(current_level, level);
                            ++diagnostic_counts;
                        });
                    }

                    auto color = diagnostic_level_to_color(std::span(config.level_to_color), current_level);

                    auto tmp_as = AnnotatedString::builder();
                    if (should_show_bullet_points) {
                        // add bullet point
                        (void)tmp_as.push(bp, {
                            .text_color = color,
                            .padding = term::PaddingValues(0, 1, 0, 0)
                        });
                        style.word_wrap_start_padding = static_cast<unsigned>(core::utf8::calculate_size(bp) + 1);

                        // Add the Diagnostic prefix the builder.
                        (void)tmp_as
                            .with_style({ .text_color = color, .dim = true })
                                .push("[")
                                .push(to_string(current_level))
                                .push("] ");
                    }

                    // Add the message.
                    (void)tmp_as.push(message);

                    // add the diagnostic indicator if more than one.
                    if (diagnostic_counts > 1) {
                        (void)tmp_as.push(" ");
                        for (auto l = diagnostic_level_elements_count; l > 0; --l) {
                            auto j = l - 1;
                            auto mask = bit_masks[j];
                            if ((info.level_bit_mask & mask) == 0) continue;
                            auto level = static_cast<DiagnosticLevel>(j);
                            (void)tmp_as.push(
                                config.square,
                                {
                                    .text_color = diagnostic_level_to_color(std::span(config.level_to_color), level)
                                }
                            );
                        }
                    }

                    style.group_id = GroupId::diagnostic_message;
                    style.trim_space = true;

                    auto new_message = tmp_as.build();
                    // render the message.
                    auto [text_container, p] = canvas.draw_text(
                        new_message,
                        x_pos,
                        y,
                        style
                    );

                    if (p != 0) {
                        auto y_max = std::min(text_container.max_y(), static_cast<unsigned>(canvas.rows()));
                        auto x_max = std::min(text_container.max_x(), static_cast<unsigned>(canvas.cols()));
                        for (auto ty = text_container.min_y(); ty < y_max; ++ty) {
                            for (auto tx = text_container.min_x(); tx < x_max; ++tx) {
                                canvas(ty, tx) = {};
                            }
                        }

                        auto tmp_style = style;
                        tmp_style.break_whitespace = false;
                        auto res = canvas.draw_text(
                            new_message,
                            x_pos,
                            y,
                            tmp_style
                        );

                        text_container = res.bbox;
                    }

                    // increase the y-coordinate by the text container height.
                    y += text_container.height;
                    auto tmp_width = text_container.width;
                    content_width = std::min(std::max(content_width, tmp_width), container.width);
                }

                // Draw box
                auto color = diagnostic_level_to_color(std::span(config.level_to_color), dominant_level);

                auto content_height = (y - container.y - 1);
                auto total_height = /*top border*/1 + /*bottom border*/1 + content_height;
                auto box = term::BoundingBox {
                    .x = x_pos,
                    .y = container.y,
                    .width = std::min(content_width, max_x - x_pos),
                    .height = total_height - 1
                };

                auto box_style = term::Style {
                    .text_color = color,
                    .group_id = style.group_id,
                    .z_index = static_cast<int>(dominant_level),
                };
                canvas.draw_box(
                    box.x,
                    box.y,
                    box.width,
                    box.height,
                    box_style,
                    config.box_normal,
                    config.box_bold
                );

                if (!should_show_bullet_points && !bp.empty()) {
                    canvas.draw_text(
                        AnnotatedString::builder()
                            .push(" ").push(to_string(dominant_level)).push(" ").build(),
                        box.x + 2,
                        box.y,
                        {
                            .text_color = box_style.text_color,
                            .group_id = box_style.group_id,
                            .z_index = box_style.z_index + 1
                        }
                    );
                }

                for (auto marker: g.spans) {
                    points.push_back({
                        marker,
                        box,
                        term::Style {
                            .text_color = color,
                            .group_id = GroupId::diagnostic_path + static_cast<unsigned>(t),
                            .z_index = static_cast<int>(dominant_level)
                        }
                    });
                }

                last_box = box.expand(6, 0);
                last_box.x = std::max(last_box.x, container.x);
                last_box.width = std::min(last_box.x, container.width);

                max_y_rendered = std::max(max_y_rendered, box.max_y() + 1);
            }
        }

        container.y = max_y_rendered + 1;

        ruler_container.y = std::min(ruler_container.y, container.y);
        while (ruler_container.y < container.y) {
            render_ruler(
                canvas,
                ruler_container,
                {},
                config.line_normal.vertical,
                config.line_normal.vertical,
                config.ruler_color
            );
            ++ruler_container.y;
        }

        return container;
    }

    static inline auto render_orphan_messages(
        term::Canvas& canvas,
        NormalizedDiagnosticAnnotations& as,
        term::BoundingBox ruler_container,
        term::BoundingBox container,
        DiagnosticRenderConfig const& config
    ) -> term::BoundingBox {
        if (as.orphans.empty()) return container;
        std::stable_sort(as.orphans.begin(), as.orphans.end(), [](DiagnosticOrphanMessageInfo const& lhs, DiagnosticOrphanMessageInfo const& rhs) {
            return static_cast<int>(lhs.level) < static_cast<int>(rhs.level);
        });

        for (auto i = 0ul; i < as.orphans.size();) {
            auto j = i + 1;
            auto level = as.orphans[i].level;
            auto color = diagnostic_level_to_color(std::span(config.level_to_color), level);
            // merge all the message that have same diagnostic level
            for (; j < as.orphans.size(); ++j) {
                if (as.orphans[j].level != as.orphans[i].level) break;
            }

            auto y = container.y + 1;
            // if there are more than 1 items show the bullet points.
            auto should_show_bullet_points = j - i > 1;
            auto content_width = dsize_t{};

            // iterate over the diagnostic messages.
            for (; i < j; ++i) {
                auto x = container.x + 2;
                auto bp = config.bullet_point;
                auto padding = unsigned{};
                if (should_show_bullet_points && !bp.empty()) {
                    canvas.draw_pixel(x, y, bp, {
                        .text_color = diagnostic_level_to_color(std::span(config.level_to_color), level),
                        .group_id = GroupId::diagnostic_orphan_message
                    });
                    padding = static_cast<dsize_t>(should_show_bullet_points) + static_cast<dsize_t>(core::utf8::calculate_size(bp));
                }
                [[maybe_unused]] auto [text_container, p] = canvas.draw_text(
                    as.messages[as.orphans[i].message_index],
                    x + padding,
                    y,
                    {
                        .group_id = GroupId::diagnostic_orphan_message,
                        .break_whitespace = true,
                        .max_width = container.width - 2
                    }
                );
                y += text_container.height;
                content_width = std::min(std::max(content_width, text_container.width + 6 + padding), container.width);
            }
            auto content_height = (y - container.y - 1);
            auto total_height = /*top border*/1 + /*bottom border*/1 + content_height;
            auto mid_point = total_height / 2;
            auto box = term::BoundingBox {
                .x = container.x - 1,
                .y = container.y,
                .width = content_width,
                .height = total_height - 1
            };
            container.y = y;

            while (ruler_container.y < box.y) {
                render_ruler(
                    canvas,
                    ruler_container,
                    {},
                    config.line_normal.vertical,
                    config.line_normal.vertical,
                    config.ruler_color
                );
                ++ruler_container.y;
            }

            for (auto m = 0ul; m < mid_point; ++m, ++ruler_container.y) {
                render_ruler(
                    canvas,
                    ruler_container,
                    {},
                    config.line_normal.vertical,
                    config.line_normal.vertical,
                    config.ruler_color
                );
            }

            if (j < as.orphans.size()) {
                canvas.draw_pixel(
                    ruler_container.width, ruler_container.y,
                    config.box_normal.right_connector,
                    {
                        .text_color = color,
                        .group_id = GroupId::diagnostic_orphan_message
                    }
                );
            } else {
                canvas.draw_pixel(
                    ruler_container.width, ruler_container.y,
                    config.line_normal.turn_up,
                    {
                        .text_color = color,
                        .group_id = GroupId::diagnostic_orphan_message
                    }
                );
            }

            canvas.draw_pixel(
                ruler_container.width + 1, box.y + mid_point,
                config.box_normal.horizonal,
                {
                    .text_color = color,
                    .group_id = GroupId::diagnostic_orphan_message
                }
            );

            canvas.draw_box(
                box.x,
                box.y,
                box.width,
                box.height,
                {
                    .text_color = color,
                    .group_id = GroupId::diagnostic_orphan_message
                },
                config.box_normal,
                config.box_bold
            );

            canvas.draw_pixel(
                ruler_container.width + 2, box.y + mid_point,
                config.box_normal.left_connector,
                {
                    .text_color = color,
                    .group_id = GroupId::diagnostic_orphan_message
                }
            );

            canvas.draw_text(
                AnnotatedString::builder()
                    .push(" ").push(to_string(level)).push(" ").build(),
                box.x + 2,
                box.y,
                {
                    .text_color = color,
                    .bold = true,
                    .group_id = GroupId::diagnostic_orphan_message
                }
            );

            if (j < as.orphans.size()) {
                container.y += 1;
                ruler_container.y += 1;
            }

            i = j;
        }

        return container;
    }

    struct DiagnosticPathGraph {
    private:
        enum class NodeState: std::uint8_t {
            Open,
            SameGroup,
            DifferentGroup,
            Blocked
        };

        enum class Direction {
            None,
            Up,
            Right,
            Down,
            Left
        };
    public:

        DiagnosticPathGraph(term::BoundingBox box)
            : m_data(box.width * box.height, NodeState::Blocked)
            , m_container(box)
        {}
        DiagnosticPathGraph(DiagnosticPathGraph const&) = delete;
        DiagnosticPathGraph(DiagnosticPathGraph &&) = delete;
        DiagnosticPathGraph& operator=(DiagnosticPathGraph const&) = delete;
        DiagnosticPathGraph& operator=(DiagnosticPathGraph &&) = delete;
        ~DiagnosticPathGraph() = default;

        constexpr auto rows() const noexcept -> unsigned { return m_container.height; }
        constexpr auto cols() const noexcept -> unsigned { return m_container.width; }

        constexpr auto operator()(std::size_t r, std::size_t c) noexcept -> NodeState& {
            assert(r >= m_container.y);
            assert(c >= m_container.x);
            return m_data[(r - m_container.y) * cols() + (c - m_container.x)];
        }

        constexpr auto operator()(std::size_t r, std::size_t c) const noexcept -> NodeState {
            assert(r >= m_container.y);
            assert(c >= m_container.x);
            return m_data[(r - m_container.y) * cols() + (c - m_container.x)];
        }

        constexpr auto init(
            term::Canvas const& canvas,
            term::Point marker,
            term::BoundingBox message,
            term::Style const& style
        ) noexcept -> void {
            std::fill(m_data.begin(), m_data.end(), NodeState::Blocked);
            m_same_group_points.clear();

            for (auto y = marker.y; y < m_container.max_y(); ++y) {
                auto x = m_container.min_x();
                for (; x < m_container.max_x(); ++x) {
                    auto const& cell = canvas(y, x);
                    if (cell.to_string() == " " || cell.empty()) {
                        this->operator()(y, x) = NodeState::Open;
                    } else {
                        break;
                    }
                }

                for (; x < m_container.max_x(); ++x) {
                    auto const& cell = canvas(y, x);
                    if (message.inside(x, y)) {
                        this->operator()(y, x) = NodeState::Blocked;
                    } else if (cell.empty()) {
                        this->operator()(y, x) = NodeState::Open;
                    } else if (cell.to_string() == " ") {
                        auto old_c = x;
                        while (x < m_container.max_x() && canvas(y, x).to_string() == " ") {
                            ++x;
                        }
                        auto len = x - old_c;
                        if (len >= 4 || cell.style.group_id == 0) {
                            this->operator()(y, x) = NodeState::Open;
                        }
                        x -= 1;
                    } else if (cell.style.group_id == style.group_id) {
                        this->operator()(y, x) = NodeState::SameGroup;
                        m_same_group_points.insert({ x, y });
                    } else if (cell.style.group_id >= GroupId::diagnostic_path) {
                        this->operator()(y, x) = NodeState::DifferentGroup;
                    }  else {
                        this->operator()(y, x) = NodeState::Blocked;
                    }
                }

                x = m_container.max_x();
                for (; x > m_container.min_x(); --x) {
                    auto const& cell = canvas(y, x - 1);
                    if (cell.to_string() == " " || cell.empty()) {
                        this->operator()(y, x) = NodeState::Open;
                    } else {
                        break;
                    }
                }
            }

            auto x_center = message.min_x() + message.width / 2;
            auto y_center = message.min_y() + message.height / 2;

            std::array connectors = {
                term::Point(message.min_x(), y_center), // left
                term::Point(x_center, message.min_y()), // top
                term::Point(message.max_x(), y_center), // right
            };

            for (auto p: connectors) {
                this->operator()(p.y, p.x) = NodeState::SameGroup;
                m_same_group_points.insert(p);
            }
        }

        void debug_print() const {
            for (auto r = m_container.min_y(); r < m_container.max_y(); ++r) {
                for (auto c = m_container.min_x(); c < m_container.max_x(); ++c) {
                    std::string_view ch = " ";
                    switch (this->operator()(r, c)) {
                    case NodeState::Open: ch = "."; break;
                    case NodeState::SameGroup: ch = "S"; break;
                    case NodeState::DifferentGroup: ch = "D"; break;
                    case NodeState::Blocked: ch = "x"; break;
                    }
                    std::print("{}", ch);
                }
                std::println();
            }
        }

        auto build_route(
            term::Canvas& canvas,
            term::Point start,
            term::Style const& style,
            DiagnosticRenderConfig const& config
        ) -> bool {
            static constexpr auto blocked = std::numeric_limits<int>::max();
            static constexpr auto turn_penality = 5;
            using node_t = std::tuple<int /*heuristic*/, term::Point, Direction>;
            std::priority_queue<node_t, std::vector<node_t>, std::greater<>> open_set{};
            std::unordered_map<term::Point, std::pair<term::Point, Direction>> path_history;
            std::unordered_map<term::Point, int> cost{};

            open_set.emplace(0, start, Direction::None);
            cost[start] = 0;

            std::array neighbours = {
                std::make_pair( 0,  1), // Down
                std::make_pair(-1,  0), // Left
                std::make_pair( 1,  0), // Right
                std::make_pair( 0, -1), // Up
            };

            // std::println("Goal: {} | {}", goals, m_container);
            while (!open_set.empty()) {
                auto [h, current, dir] = open_set.top();
                open_set.pop();

                // canvas.draw_pixel(current.x, current.y, ".");
                if (is_goal(current)) {
                    core::SmallVec<term::Point, 0> pts;
                    for (auto p = current; p != start; p = path_history[p].first) {
                        pts.push_back(p);
                        m_same_group_points.insert(p);
                        this->operator()(p.y, p.x) = NodeState::SameGroup;
                    }
                    pts.push_back(start);
                    m_same_group_points.insert(start);

                    start.y -= 1;
                    pts.push_back(start);
                    m_same_group_points.insert(start);

                    canvas.draw_path(pts, style);

                    auto arrow_style = style;
                    arrow_style.group_id = GroupId::diagnostic_message;
                    canvas.draw_pixel(
                        start.x,
                        start.y,
                        config.arrow_bold.up,
                        arrow_style
                    );

                    auto connector = std::string_view{};

                    switch (dir) {
                        case Direction::Up: connector = config.box_normal.bottom_connector; break; 
                        case Direction::Down: connector = config.box_normal.top_connector; break;
                        case Direction::Left: connector = config.box_normal.right_connector; break;
                        case Direction::Right: connector = config.box_normal.left_connector; break;
                        default: break;
                    }

                    if (!connector.empty()) {
                        canvas.draw_pixel(
                            current.x, current.y,
                            connector,
                            style
                        );
                    }

                    return true;
                }

                for (auto [dx, dy]: neighbours) {
                    auto next = term::Point(
                        static_cast<unsigned>(static_cast<int>(current.x) + dx),
                        static_cast<unsigned>(static_cast<int>(current.y) + dy)
                    );
                    auto c = cell_cost(next.x, next.y);
                    if (c == blocked) continue;
                    auto new_dir = get_direction(current, next);
                    auto old_cost = cost[current];
                    auto new_cost = old_cost + c;

                    if (new_dir != Direction::None && new_dir != dir) {
                        new_cost += turn_penality;
                    }

                    if (!cost.contains(next) || new_cost < cost[next]) {
                        cost[next] = new_cost;
                        auto new_h = cal_heuristic(next);
                        open_set.emplace(new_cost + new_h, next, new_dir);
                        path_history[next] = { current, new_dir };
                    }
                }
            }

            return false;
        }
    private:

        static constexpr auto get_direction(term::Point cur, term::Point to) noexcept -> Direction {
            if (cur.x == to.x) {
                if (cur.y - 1 == to.y) return Direction::Up;
                if (cur.y + 1 == to.y) return Direction::Down;
            } else if (cur.y == to.y) {
                if (cur.x - 1 == to.x) return Direction::Left;
                if (cur.x + 1 == to.x) return Direction::Right;
            }
            return Direction::None;
        }

        static constexpr auto dist(term::Point s, term::Point e) noexcept -> int {
            auto x1 = static_cast<int>(s.x);
            auto y1 = static_cast<int>(s.y);
            auto x2 = static_cast<int>(e.x);
            auto y2 = static_cast<int>(e.y);
            return std::abs(x2 - x1) + std::abs(y2 - y1);
        }

        static constexpr auto contains(std::span<term::Point> goals, term::Point p) noexcept -> bool {
            return std::find(goals.begin(), goals.end(), p) != goals.end();
        }

        constexpr auto is_goal(term::Point p) const noexcept -> bool {
            if (p.x >= m_container.max_x() || p.y >= m_container.max_y()) return false;
            if (p.x < m_container.x || p.y < m_container.y) return false;
            if (m_same_group_points.contains(p)) return true;
            return this->operator()(p.y, p.x) == NodeState::SameGroup;
        }

        constexpr auto cell_cost(unsigned x, unsigned y) const noexcept -> int {
            static constexpr auto blocked = std::numeric_limits<int>::max();
            if (x >= m_container.max_x() || y >= m_container.max_y()) return blocked;
            if (x < m_container.x || y < m_container.y) {
                return blocked;
            }
            auto const& cell = this->operator()(y, x);
            switch (cell) {
            case NodeState::Open: return 1;
            case NodeState::SameGroup: return 1;
            case NodeState::DifferentGroup: return 10;
            default: return blocked;
            }
        }

        constexpr auto cal_heuristic(term::Point p) const noexcept -> int {
            auto d = std::numeric_limits<int>::max();
            for (auto [x, y]: m_same_group_points) {
                d = std::min(d, dist(p, term::Point(x, y)));
            }
            return d;
        }
    private:
        std::vector<NodeState> m_data;
        term::BoundingBox m_container;
        std::unordered_set<term::Point> m_same_group_points;
    };

    static inline auto render_path(
        term::Canvas& canvas,
        term::BoundingBox container,
        point_container_t& points,
        DiagnosticRenderConfig const& config
    ) noexcept -> void {
        if (points.empty()) return;
        // The first pass: render the simple/straight paths.
        {
            // Simple paths:
            //   1. No intersection
            //   2. Marker and message lies in a single line, no turns.

            // if any message cross the this x-coordinate, it is guaranteed to intersect.
            // This constraint is guaranteed by the construction.
            auto max_x_position = std::get<0>(points[0]).x + 1;
            auto rendered_count = std::size_t{};
            for (auto& [marker_pt, message_box, style]: points) {
                if (marker_pt == term::Point(0, 0)) continue;
                auto x_min = std::min(message_box.x, max_x_position);
                auto x_max = std::min(message_box.bottom_right().first, max_x_position);
                if (x_max - x_min == 0) continue;
                if (!(marker_pt.x >= x_min && marker_pt.x < x_max)) continue;

                auto arrow_pt = term::Point(marker_pt.x, marker_pt.y + 1);
                auto box_pt = term::Point(marker_pt.x, message_box.y);

                bool has_intersections{false};
                for (auto y = arrow_pt.y; y < box_pt.y; ++y) {
                    auto const& cell = canvas(y, box_pt.x);
                    if (cell.style.group_id != 0) {
                        has_intersections = true;
                        break;
                    }
                }

                if (has_intersections) continue;

                // render straight line
                canvas.draw_line(
                    box_pt.x, box_pt.y,
                    arrow_pt.x, arrow_pt.y,
                    style,
                    /*top_bias=*/false,
                    config.line_normal,
                    config.line_bold
                );

                // render connector
                if (box_pt.x > message_box.x && box_pt.x < message_box.bottom_right().first) {
                    canvas.draw_pixel(
                        box_pt.x, box_pt.y,
                        config.box_normal.top_connector,
                        style
                    );
                }

                // render arrow
                {
                    auto arrow_style = style;
                    arrow_style.group_id = GroupId::diagnostic_message;
                    canvas.draw_pixel(
                        arrow_pt.x, arrow_pt.y,
                        config.arrow_bold.up,
                        arrow_style
                    );
                }

                marker_pt = {0, 0};
                max_x_position = std::min(x_min + 1, max_x_position);
                ++rendered_count;
            }

            if (points.size() == rendered_count) return;
        }

        points.erase(std::remove_if(points.begin(), points.end(), [](auto const& el) {
            return std::get<0>(el).x == 0;
        }), points.end());

        // The last pass: find the complex using A* alogrithm paths and then render.
        for (auto i = 0ul; i < points.size();) {
            auto [marker, message, style] = points[i];
            marker.y += 2;

            auto graph = DiagnosticPathGraph(container);
            graph.init(canvas, marker, message, style);
            graph.build_route(canvas, marker, style, config);

            auto j = i + 1;
            // join all the markers using the previously drawn path.
            while (j < points.size()) {
                auto p = points[j - 1];
                auto c = points[j];
                if (std::get<2>(p).group_id != std::get<2>(c).group_id) {
                    break;
                }

                marker = std::get<0>(c);
                marker.y += 2;

                graph.build_route(canvas, marker, style, config);
                ++j;
            }

            i = j;
        }
    }
} // namespace dark::internal

namespace dark {
    template <typename T>
    static inline auto render_diagnostic(
        Terminal<T>& term,
        Diagnostic& diag,
        DiagnosticRenderConfig const& config = {}
    ) -> void {
        using namespace internal;

        auto canvas = term::Canvas(term.columns());
        auto bbox = render_diagnostic_message(canvas, diag, config);
        auto line_number_width = static_cast<unsigned>(calculate_max_number_line_width(diag)) + 1;
        bbox = render_file_info(
            canvas,
            diag,
            line_number_width,
            bbox.bottom_left().second,
            config
        );

        auto ruler_container = term::BoundingBox {
            .x = 1,
            .y = bbox.height,
            .width = line_number_width,
            .height = 1
        };

        auto content_container = term::BoundingBox {
            .x = ruler_container.max_x() + 2,
            .y = bbox.height,
            .width = static_cast<unsigned>(canvas.cols()) - ruler_container.max_x() - 3,
            .height = ~unsigned{0}
        };

        auto annotations = normalize_diagnostic_messages(diag);

        auto message_markers = message_marker_t{};
        content_container = render_source_text(
            canvas,
            diag,
            annotations,
            ruler_container,
            content_container,
            message_markers,
            config
        );

        ruler_container.y = content_container.y;

        point_container_t points{};
        content_container = render_span_messages(
            canvas,
            annotations,
            ruler_container,
            content_container,
            message_markers,
            points,
            config
        );

        auto allowed_path_container = term::BoundingBox {
            .x = ruler_container.max_x(),
            .y = bbox.height,
            .width = static_cast<unsigned>(canvas.cols()) - ruler_container.max_x(),
            .height = content_container.y - bbox.height
        };
        ruler_container.y = content_container.y;

        render_path(canvas, allowed_path_container, points, config);

        render_orphan_messages(
            canvas,
            annotations,
            ruler_container,
            content_container,
            config
        );


        canvas.render(term);
    }
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_RENDERER_HPP
