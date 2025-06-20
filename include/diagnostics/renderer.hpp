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
        case DiagnosticLevel::Remark: return Color::Green;
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
        case DiagnosticLevel::Remark: return "E";
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
                auto [l, ls] = lhs.strings[i];
                auto [r, rs] = rhs.strings[i];
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

        for (auto& tok: line.tokens) {

            core::SmallVec<std::size_t> insert_indices{};

            // 1. Find the first match.
            std::size_t start{};
            for (; start < an.spans.size(); ++start) {
                // Since `an.spans` are sorted by start we can use the first match, and from
                // there, we find all the spans that are within the bounds.
                auto const& info = an.spans[start];
                auto span = info.span;
                if (info.level != DiagnosticLevel::Insert) continue;
                if (!tok.span().is_between(span.start())) break;
            }
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

                auto first_marker = Span::from_size(tok.marker.start(), std::min(tok.marker.size(), first.size()));
                auto last_marker = Span(first_marker.end(), tok.marker.end());

                if (!first.empty()) {
                    last.tokens.push_back(NormalizedDiagnosticTokenInfo {
                        .text = tok.text.substr(0, first.size()),
                        .markers = { { .kind = MarkerKind::Primary, .span = first_marker } },
                        .token_start_offset = tok.token_start_offset,
                        .text_color = tok.text_color,
                        .bg_color = tok.bg_color,
                        .bold = tok.bold,
                        .italic = tok.italic
                    });
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
                                last.tokens.push_back(
                                    NormalizedDiagnosticTokenInfo {
                                        .text = std::move(itok.text),
                                        .markers = {
                                            DiagnosticMarker {
                                                .kind = MarkerKind::Insert,
                                                .span = top.span,
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
                        .markers = { { .kind = MarkerKind::Primary, .span = last_marker } },
                        .token_start_offset = tok.token_start_offset,
                        .text_color = tok.text_color,
                        .bg_color = tok.bg_color,
                        .bold = tok.bold,
                        .italic = tok.italic
                    });
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
                    auto first = Span::from_size(top.span.start(), std::min(top.span.size(), span.size()));
                    auto end = Span(first.start(), top.span.end());

                    auto kind = MarkerKind::Secondary;
                    if (top.level == DiagnosticLevel::Delete) kind = MarkerKind::Delete;

                    // Insert the first span and overflowing span will be inserted to the next intersecting tokens.
                    el.markers.push_back({ .kind = kind, .span = first, .annotation_index = static_cast<unsigned>(j) });

                    // insert the remaining annotation span to the remaining tokens.
                    for (auto k = i + 1; k < l.tokens.size() && !end.empty(); ++k) {
                        auto& tmp = l.tokens[k];
                        if (tmp.is_artificial) continue;
                        if (!tmp.span().is_between(end.start())) break;
                        first = Span::from_size(end.start(), std::min(end.size(), tmp.span().size()));
                        end = Span(first.start(), end.end());
                        tmp.markers.push_back({ .kind = kind, .span = first, .annotation_index = static_cast<unsigned>(j) });
                    }
                }
            }
        }
        for (auto& l: res) {
            for (auto& el: l.tokens) {
                auto size = el.markers.size();
                // 1. [1, 0, 2, 3] -> size = 4
                // 2. [1, 3, 2] -> size = 3
                for (auto i = 0ul; i < size; ++i) {
                    if (el.markers[i].span.empty()) {
                        --size;
                        std::swap(el.markers[i], el.markers[size]);
                    }
                }

                while (size < el.markers.size()) {
                    el.markers.pop_back();
                }

                std::stable_sort(el.markers.begin(), el.markers.end(), [](DiagnosticMarker const& l, DiagnosticMarker const& r) {
                    return l.span.start() < r.span.start();
                });

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

        // Case 6: Overlapping markers
        //  Ex:
        //    | let xy = func(a, b);           |
        //    |       ~~~~~~~~                 |
        //    |       ~~~~~~~~~~~~~            |
        //    |       ~~~~~~~                  |

            auto normalized_tokens = normalize_diagnostic_line(line, as);

            using buffer_t = std::tuple<
                unsigned /*token index*/,
                unsigned /*text index*/,
                bool /*is_skippable*/
            >;
            std::vector<buffer_t> buffer(canvas.cols() + 1);

            auto buff_size = std::size_t{};

            auto try_render_line = [&buff_size, &buffer](
                std::span<NormalizedDiagnosticTokenInfo> const& tokens,
                unsigned token_index,
                unsigned text_index
            ) -> std::tuple<bool, unsigned, unsigned> {
                buff_size = 0;
                for (; token_index < tokens.size(); ++token_index) {
                    auto const& token = tokens[token_index];
                    auto text = token.text.to_borrowed();
                    if (text.empty()) continue;

                    auto span = Span();
                    for (auto const& m: token.markers) {
                        span = m.span.force_merge(span);
                    }
                    auto old_buff_size = buff_size;
                    for (; text_index < text.size();) {
                        auto len = core::utf8::get_length(text[text_index]);
                        assert(text.size() >= text_index + len);

                        auto is_skippable = !token.is_artificial || (!span.empty() && span.is_between(static_cast<dsize_t>(text_index)));
                        buffer[buff_size++] = { token_index, text_index, is_skippable };
                        if (text[text_index] == '\t') {
                            for (auto i = 1ul; i < tab_width ; ++i) {
                                buffer[buff_size++] = { token_index, text_index, is_skippable };
                            }
                        }
                        if (buff_size >= buffer.size()) {
                            buff_size = old_buff_size;
                            return { false, token_index, text_index };
                        }
                        text_index += len;
                    }
                }

                return { true, token_index, text_index };
            };

            x = container.x;

            for (auto& line_of_tokens: normalized_tokens) {
                auto render_result = std::make_tuple(false, 0u, 0u);
                do {
                    render_result = try_render_line(line_of_tokens.tokens, 0, 0);
                    auto [success, token_index, text_index] = render_result;

                    if (success) {
                        auto free_space = buffer.size() - buff_size;
                        auto start_padding = std::min<std::size_t>(
                            std::max(free_space, std::size_t{1}) - 1,
                            line_of_tokens.tokens[0].token_start_offset - line.line_start_offset
                        );
                        x += static_cast<unsigned>(start_padding);
                        auto bottom_padding = 0u;
                        for (auto const& token: line_of_tokens.tokens) {
                            auto text = token.text.to_borrowed();
                            if (text.empty()) continue;

                            auto marker_start = text.size();
                            if (!token.markers.empty()) {
                                marker_start = token.markers[0].span.start();
                            }

                            // Render before marker
                            for (auto i = 0ul; i < marker_start; ++x) {
                                auto len = core::utf8::get_length(text[i]);
                                assert(text.size() >= i + len);
                                auto txt = text.substr(i, len);

                                canvas.draw_pixel(
                                    x, y,
                                    txt,
                                    token.to_style()
                                );
                                i += len;
                            }

                            auto marker_end = marker_start;
                            for (auto i = 0ul; i < token.markers.size(); ++i) {
                                auto const& m = token.markers[i];
                                marker_end = std::max<std::size_t>(marker_end, m.span.end());
                                
                            }

                            // Render after the marker
                            for (auto i = marker_end; i < text.size(); ++x) {
                                auto len = core::utf8::get_length(text[i]);
                                assert(text.size() >= i + len);
                                auto txt = text.substr(i, len);

                                canvas.draw_pixel(
                                    x, y,
                                    txt,
                                    token.to_style()
                                );
                                i += len;
                            }
                        }
                        y += bottom_padding;
                        break;
                    }
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
