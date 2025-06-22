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
#include <__ostream/print.h>
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <optional>
#include <print>
#include <string_view>
#include <unordered_map>

namespace dark {
    struct DiagnosticRenderConfig {
        term::BoxCharSet box_normal{ term::char_set::box::rounded };
        term::BoxCharSet box_bold{ term::char_set::box::rounded_bold };
        term::LineCharSet line_normal{ term::char_set::line::rounded };
        term::LineCharSet line_bold{ term::char_set::line::rounded_bold };
        term::ArrowCharSet array_normal{ term::char_set::arrow::basic };
        term::ArrowCharSet array_bold{ term::char_set::arrow::basic_bold };
        std::string_view dotted_vertical{ term::char_set::line::dotted.vertical };
        std::string_view dotted_horizontal{ term::char_set::line::dotted.horizonal };
        unsigned max_non_marker_lines{4};
        unsigned diagnostic_kind_padding{4};
        std::string_view primary_marker{"^"};
        std::string_view secondary_marker{"~"};
    };
} // namespace dark

namespace dark::internal {
    struct GroupId {
        static constexpr unsigned diagnostic_message = 1;
        static constexpr unsigned diagnostic_ruler = 2;
        static constexpr unsigned diagnostic_annotation_base_offset = 3;
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

    constexpr auto diagnostic_level_to_color(DiagnosticLevel level) noexcept -> Color {
        switch (level) {
        case DiagnosticLevel::Help: return Color::Green;
        case DiagnosticLevel::Note: return Color::Blue;
        case DiagnosticLevel::Warning: return Color::Yellow;
        case DiagnosticLevel::Error: return Color::Red;
        case DiagnosticLevel::Insert: return Color::Green;
        case DiagnosticLevel::Delete: return Color::Red;
          break;
        }
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

    struct NormalizedDiagnosticAnnotations {
        using diagnostic_index_t = std::size_t;
        // [(AnnotatedString, min span start)]
        core::SmallVec<std::pair<term::AnnotatedString, dsize_t>> messages{};
        core::SmallVec<DiagnosticMessageSpanInfo> spans{};
        std::unordered_map<diagnostic_index_t, DiagnosticSourceLocationTokens> tokens{};

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
                    el.first
                )) {
                    message_id = m;
                }
            }

            // 2. If not found, insert the current one
            if (message_id == DiagnosticMessageSpanInfo::npos) {
                message_id = res.messages.size();
                res.messages.emplace_back(std::move(annotation.message), std::numeric_limits<dsize_t>::max());
            }

            // 3. Store tokens inside the hashmap
            res.tokens[i] = std::move(annotation.tokens);

            // 4. Store spans with ids and filter out out-of-bound spans
            for (auto span: annotation.spans) {
                if (!source_span.is_between(span, true)) continue;
                // 5. store min span start with annotation message.
                auto& start = res.messages[message_id].second;
                start = std::min(start, span.start());

                res.spans.push_back(DiagnosticMessageSpanInfo {
                    .message_index = message_id,
                    .diagnostic_index = i,
                    .level = annotation.level,
                    .span = span
                });
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
            .text_color = diagnostic_level_to_color(diag.level),
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
        Color color = Color::Blue
    ) -> term::BoundingBox {
        auto width = container.width;
        auto y = container.y;
        auto line = line_number.has_value() ? vertical : dotted_vertical;
        auto num = line_number
            .transform([width](dsize_t n) -> std::string {
                return std::format("{:>{}}", n, std::max(width, dsize_t{2}) - 2);
            })
            .value_or(std::string());

        canvas.draw_text(num, container.x, y, { .text_color = color, .bold = true });
        if (line.empty()) {
            line = "|";
            if (!line_number.has_value()) {
                canvas.draw_text("...", 1, y, { .text_color = color, .bold = true });
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
            .text_color = Color::Blue
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
        box.height += 1;
        return box;
    }

    enum class MarkerKind {
        Primary,
        Secondary,
        Insert,
        Delete,
    };

    struct DiagnosticMarker {
        static constexpr std::string_view primary   = "^";
        static constexpr std::string_view quad      = "≣";
        static constexpr std::string_view tripple   = "≋";
        static constexpr std::string_view double_   = "≈";
        static constexpr std::string_view single    = "~";
        static constexpr std::string_view circle    = "●";
        static constexpr std::string_view remove    = "x";
        static constexpr std::string_view add       = "+";
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
        core::SmallVec<std::size_t> insert_indices{};

        for (auto& tok: line.tokens) {
            insert_indices.clear();

            // 1. Find the first match.
            std::size_t start{};
            for (; start < an.spans.size(); ++start) {
                // Since `an.spans` are sorted by start we can use the first match, and from
                // there, we find all the spans that are within the bounds.
                auto const& info = an.spans[start];
                auto span = info.span;
                if (info.level != DiagnosticLevel::Insert) continue;
                if (tok.span().is_between(span.start())) break;
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
                insert_indices.push_back(start);
            }

            // Case 1:
            //   |--------- Token ----------|
            //      |---- span 1-------|
            //      |--span 2---|
            // Case 2:
            //   |--------- Token ----------|
            //      |---- span 1-------|
            //         |--span 2---|

            std::reverse(insert_indices.begin(), insert_indices.end());
            while (!insert_indices.empty()) {
                auto& top = an.spans[insert_indices.back()];
                auto span = tok.span();

                auto first = Span::from_size(span.start(), top.span.start());
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

                while (!insert_indices.empty() && an.spans[insert_indices.back()].span.start() == top.span.start()) {
                    auto annotation_index = insert_indices.back();
                    top = an.spans[annotation_index];
                    insert_indices.pop_back();

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
                                                .annotation_index = static_cast<unsigned>(annotation_index)
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

                std::stable_sort(el.markers.begin(), el.markers.end(), [](DiagnosticMarker const& l, DiagnosticMarker const& r) {
                    return l.span.start() < r.span.start();
                });

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
                    // TODO: improve this by find the lower bound and insert afte that position.
                    el.markers.push_back(lhs);
                    std::stable_sort(el.markers.begin(), el.markers.end(), [](DiagnosticMarker const& l, DiagnosticMarker const& r) {
                        return l.span.start() < r.span.start();
                    });
                }

                // for (auto const& item: el.markers) {
                //     std::println("[{}, {}, {}], ", item.annotation_index, item.span, item.is_start);
                // }

                // make the spans relative
                for (auto& m: el.markers) {
                    m.span = m.span - el.token_start_offset;
                }
            }
        }

        return res;
    }

    static inline auto render_source_text(
        term::Canvas& canvas,
        Diagnostic& diag,
        NormalizedDiagnosticAnnotations& as,
        term::BoundingBox ruler_container,
        term::BoundingBox container,
        DiagnosticRenderConfig const& config
    ) noexcept -> term::BoundingBox {
        static constexpr auto tab_width = term::Canvas::tab_width;
        char tab_indent_buff[tab_width] = {' '};
        std::fill_n(tab_indent_buff, tab_width, ' ');
        auto tab_indent = std::string_view(tab_indent_buff, tab_width);
        static_assert(tab_width > 0);

        auto& lines = diag.location.source.lines;
        auto x = container.x;
        auto y = container.y;

        auto skip_check_for = 0ul;

        constexpr auto count_whitespace_len = [](auto const& tokens) -> unsigned {
            auto count = 0u;
            for (auto const& tok: tokens) {
                auto text = tok.text.to_borrowed();
                auto idx = text.find_first_not_of(" \t");
                if (idx == std::string_view::npos) break;
                for (auto i = 0ul; i < idx; ++count) {
                    auto len = core::utf8::get_length(text[i]);
                    if (text[i] == '\t') count += tab_width - 1;
                    i += len;
                }
                if (idx != text.size()) {
                    break;
                }
            }
            return count;
        };

        // Point -> annotation index
        std::unordered_map<term::Point, core::SmallVec<DiagnosticMarker, 2>> marker_to_message;

        for (auto l = 0ul; l < lines.size();) {
            auto& line = lines[l];
            ruler_container.y = y;

            // Adds ellipsis if consecutive non-marked lines are present, and it exceeds `max_non_marker_lines` from config
            if (!line.has_any_marker() && skip_check_for == 0) {
                auto number_of_non_marker_lines = 0ul;
                auto last_whitespace_count = 0u;
                auto last_span_start = unsigned{};
                auto old_l = l;

                while (l < lines.size() && !lines[l].has_any_marker()) {
                    auto& t_line = lines[l];
                    // calculate line start so we can use it for start of "..."
                    last_whitespace_count = count_whitespace_len(t_line.tokens);
                    last_span_start = t_line.rel_span().start();
                    ++l;
                    ++number_of_non_marker_lines;
                }

                if (number_of_non_marker_lines >= config.max_non_marker_lines) {
                    if (l >= lines.size()) break;
                    render_ruler(
                        canvas,
                        ruler_container,
                        {},
                        "",
                        config.dotted_vertical
                    );
                    auto new_x = static_cast<unsigned>(x + last_whitespace_count + last_span_start);
                    new_x = std::min(new_x, static_cast<unsigned>(canvas.cols()) - 3);
                    canvas.draw_text(
                        "...",
                        new_x,
                        y,
                        { .bold = true, }
                    );
                    y++;
                    continue;
                }

                skip_check_for = l - old_l;
                l = old_l;
            } else {
                skip_check_for = std::max(skip_check_for, 1ul) - 1;
            }

            render_ruler(
                canvas,
                ruler_container,
                line.line_number,
                "",
                config.dotted_vertical
            );

            ++l;
            if (line.tokens.empty()) continue;

            auto normalized_tokens = normalize_diagnostic_line(line, as);

            std::size_t total_canvas_cols = canvas.cols();
            auto cols_occupied = std::size_t{};

            auto try_render_line = [&cols_occupied, total_canvas_cols](
                std::span<NormalizedDiagnosticTokenInfo> const& tokens
            ) -> bool {
                cols_occupied = 0;
                auto success = true;
                for (auto token_index = 0ul; token_index < tokens.size(); ++token_index) {
                    auto const& token = tokens[token_index];
                    auto text = token.text.to_borrowed();
                    if (text.empty()) continue;

                    for (auto text_index = 0ul; text_index < text.size();) {
                        auto len = core::utf8::get_length(text[text_index]);
                        assert(text.size() >= text_index + len);

                        cols_occupied += text[text_index] == '\t' ? tab_width : 1u;

                        if (cols_occupied >= total_canvas_cols) {
                            success = false;
                        }
                        text_index += len;
                    }
                }

                return success;
            };

            x = container.x;

            for (auto& line_of_tokens: normalized_tokens) {
                auto render_result = std::make_tuple(false, 0u, 0u);
                do {
                    auto success = try_render_line(line_of_tokens.tokens);

                    if (success) {
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
                            line_of_tokens.tokens[0].token_start_offset - line.line_start_offset
                        );
                        x += static_cast<unsigned>(start_padding);
                        auto bottom_padding = 0u;
                        for (auto const& token: line_of_tokens.tokens) {
                            auto token_x_pos = x;

                            auto text = token.text.to_borrowed();
                            if (text.empty()) continue;

                            auto marker_start = text.size();
                            if (!token.markers.empty()) {
                                marker_start = token.markers[0].span.start();
                            }

                            auto render_text = [text, &canvas, tab_indent](
                                std::size_t start,
                                std::size_t end,
                                unsigned x,
                                unsigned y,
                                term::Style style
                            ) -> unsigned {
                                for (auto i = start; i < end; ++x) {
                                    auto len = core::utf8::get_length(text[i]);
                                    assert(text.size() >= i + len);
                                    auto txt = text.substr(i, len);

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
                                    i += len;
                                }
                                return x;
                            };

                            // Render before marker
                            x = render_text(0, marker_start, x, y, token.to_style());

                            auto marker_end = marker_start;
                            auto m_size = token.markers.size();
                            for (auto i = 0ul; i < m_size;) {

                                auto j = i + 1;
                                while (
                                    j < m_size &&
                                    token.markers[j].span.start() == token.markers[i].span.start()
                                ) {
                                    ++j;
                                }

                                // Positions of marker:
                                // Primary Marker > Error > Warning >  
                                std::array<std::pair<unsigned, unsigned>, diagnostic_level_elements_count> marker_freq{};
                                std::fill(marker_freq.begin(), marker_freq.end(), std::make_pair(0, std::numeric_limits<unsigned>::max()));
                                unsigned marker_rel_pos{};

                                marker_start = token.markers[i].span.start();
                                // 1. precompute markers and set non-secondary markers' relative position to 0.
                                {
                                    auto primary_span = Span();
                                    for (auto k = i; k < j; ++k) {
                                        auto const& m = token.markers[k];
                                        if (m.kind == MarkerKind::Primary) {
                                            primary_span = m.span;
                                            break;
                                        }
                                    }

                                    for (auto k = i; k < j; ++k) {
                                        auto const& m = token.markers[k];
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
                                core::SmallVec<std::array<unsigned, diagnostic_level_elements_count>, 64> marker_count_for_each_cell(marker_end + 1);

                                // increment the markers in each cells
                                for (auto k = i; k < j; ++k) {
                                    auto const& m = token.markers[k];
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

                                // 2. store the span and render marker
                                for (; i < j; ++i) {
                                    auto const& m = token.markers[i];
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
                                        y + 1u + rel_y_pos
                                    );
                                    if (m.is_start) {
                                        auto& marker_to_message_item = marker_to_message[pt];
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
                                        auto color = diagnostic_level_to_color(d_level);
                                        auto style = token.to_style();

                                        switch (m.kind) {
                                            case MarkerKind::Primary: {
                                                marker = DiagnosticMarker::primary;
                                                z_index += 50;
                                            } break;
                                            case MarkerKind::Secondary: {
                                                auto count = marker_count_for_each_cell[pos][static_cast<unsigned>(d_level)];
                                                switch (count) {
                                                    case 0: case 1: marker = DiagnosticMarker::single; break;
                                                    case 2: marker = DiagnosticMarker::double_; break;
                                                    case 3: marker = DiagnosticMarker::tripple; break;
                                                    default: marker = DiagnosticMarker::quad; break;
                                                }
                                            } break;
                                            case MarkerKind::Insert: {
                                                marker = DiagnosticMarker::add;
                                                z_index += 50;
                                                style.text_color = color;
                                                style.bold = true;
                                            } break;
                                            case MarkerKind::Delete: {
                                                marker = DiagnosticMarker::remove; 
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
                                        for (auto k = 0ul; k < iter; ++k, ++tx) {
                                            canvas.draw_pixel(
                                                tx,
                                                y,
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
                                    }
                                    x = std::max(tx, x);
                                }

                                bottom_padding = std::max(marker_rel_pos, bottom_padding);

                                // Render non-marked fragments
                                auto new_marker_end = j < m_size ? token.markers[j].span.start() : marker_end;
                                x = render_text(marker_end, new_marker_end, x, y, token.to_style()); 
                            }

                            // Render after the marker
                            x = render_text(marker_end, text.size(), x, y, token.to_style());
                        }
                        for (auto i = 0u; i < bottom_padding; ++i) {
                            auto tmp = ruler_container;
                            tmp.y++;
                            render_ruler(
                                canvas,
                                tmp,
                                {},
                                "",
                                config.dotted_vertical
                            );
                        }
                        y += bottom_padding;
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
                    // 3. start removing from smallest distance.

                } while (!std::get<0>(render_result));
            }

            ++y;
        }
        container.y = y;
        return container;
    }
} // namespace dark::internal

namespace dark {
    static inline auto render_diagnostic(
        Terminal& term,
        Diagnostic& diag,
        DiagnosticRenderConfig config = {}
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
            .x = ruler_container.top_right().first + 2,
            .y = bbox.height,
            .width = static_cast<unsigned>(canvas.cols()),
            .height = ~unsigned{0}
        };
        content_container.width -= content_container.x + 1;

        auto annotations = normalize_diagnostic_messages(diag);

        render_source_text(
            canvas,
            diag,
            annotations,
            ruler_container,
            content_container,
            config
        );

        canvas.render(term);
    }
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_RENDERER_HPP
