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
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <limits>
#include <print>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace dark {
    struct DiagnosticRenderConfig {
        term::BoxCharSet box_normal{ term::char_set::box::rounded };
        term::BoxCharSet box_bold { term::char_set::box::rounded_bold };
        term::LineCharSet line_normal { term::char_set::line::rounded };
        term::LineCharSet line_bold { term::char_set::line::rounded_bold };
        term::ArrowCharSet array_normal { term::char_set::arrow::basic };
        term::ArrowCharSet array_bold { term::char_set::arrow::basic_bold };
        std::string_view dotted_vertical{ term::char_set::line::dotted.vertical };
        std::string_view dotted_horizontal{ term::char_set::line::dotted.horizonal };
        unsigned max_non_marker_lines{4};
        unsigned diagnostic_kind_padding{4};
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
                if (!source_span.is_between(span)) continue;
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
            { .groupId = GroupId::diagnostic_message, .word_wrap = true, .break_whitespace = true }
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
        canvas.draw_pixel(width, y, line, { .text_color = color, .groupId = GroupId::diagnostic_ruler });
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
        auto box = canvas.draw_text(std::move(an), x, y, { .groupId = GroupId::diagnostic_message }).bbox;
        canvas.draw_pixel(
            box.x,
            box.bottom_left().second,
            config.line_normal.vertical,
            { .text_color = *line_style.text_color, .groupId = GroupId::diagnostic_message }
        );
        box.height += 1;
        return box;
    }

    static inline auto render_source_text(
        term::Canvas& canvas,
        Diagnostic& diag,
        NormalizedDiagnosticAnnotations const& as,
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

            // split tokens by '\t'
            auto tokens = core::SmallVec<DiagnosticTokenInfo>{};
            tokens.reserve(line.tokens.size());

            for (auto t = 0ul; t < line.tokens.size(); ++t) {
                auto& tok = line.tokens[t];
                auto text = tok.text.to_borrowed();
                if (text.size() == 1) {
                    tokens.push_back(std::move(tok));
                    continue;
                }
                auto index = text.find("\t");
                if (index == std::string_view::npos) {
                    tokens.push_back(std::move(tok));
                    continue;
                }
                auto marker = tok.marker;
                auto make_token = [&tok](std::string_view text, Span marker, dsize_t token_offset) {
                    auto tmp = DiagnosticTokenInfo {
                        .text = "",
                        .token_start_offset = token_offset,
                        .marker = marker,
                        .text_color = tok.text_color,
                        .bg_color = tok.bg_color,
                        .bold = tok.bold,
                        .italic = tok.italic
                    };

                    if (tok.text.is_borrowed()) {
                        tmp.text = text;
                    } else {
                        tmp.text = core::CowString(std::string(text));
                    }
                    return tmp;
                };

                do {
                    auto tab_start = index;
                    auto tab_end = index + 1;
                    for (auto i = tab_end; i < text.size(); ++i, ++tab_end) {
                        if (text[i] != '\t') break;
                    }

                    auto first_marker = Span();
                    auto tab_marker = Span();

                    auto first = text.substr(0, tab_start);
                    auto tab = text.substr(tab_start, tab_end - tab_start);
                    text = text.substr(tab_end);

                    auto first_token_offset = tok.token_start_offset;
                    auto tab_token_offset = static_cast<dsize_t>(first_token_offset + first.size());
                    auto rem_token_offset = static_cast<dsize_t>(tab_token_offset + tab.size());
                    tok.token_start_offset = rem_token_offset;

                    if (!marker.empty()) {
                        auto start = marker.start() - first_token_offset;
                        auto size = marker.size();

                        if (start < tab_start) {
                            auto tmp_start = start + first_token_offset;
                            first_marker = Span::from_size(
                                tmp_start,
                                static_cast<dsize_t>(tab_start - start)
                            );
                            size -= first_marker.size(); 
                        }

                        tab_marker = Span::from_size(
                            tab_token_offset,
                            std::min(size, static_cast<dsize_t>(tab.size()))
                        );
                        size -= tab_marker.size();

                        marker = Span::from_size(
                            rem_token_offset,
                            size
                        );
                    }

                    if (!first.empty()) {
                        tokens.push_back(make_token(first, first_marker, first_token_offset));
                    }

                    tokens.push_back(make_token(
                        tab, tab_marker,
                        tab_token_offset
                    ));

                    index = text.find("\t");
                } while (index != std::string_view::npos);

                if (!text.empty()) {
                    tokens.push_back(make_token(text, marker, tok.token_start_offset));
                }
            }

            line.tokens = std::move(tokens);

            auto annotated_line = AnnotatedString{};
            auto bottom_padding = 0u;

            for (auto const& tok: line.tokens) {
                auto text = tok.text.to_borrowed();
                if (!tok.marker.empty()) {
                    bottom_padding = 1;
                }

                auto marker = Span::from_size(
                    tok.marker.start() - tok.token_start_offset,
                    tok.marker.size()
                );

                auto span_style = SpanStyle {
                    .text_color = tok.text_color,
                    .bg_color = tok.bg_color,
                    .bold = tok.bold,
                    .italic = tok.italic,
                };

                if (marker.empty()) {
                    annotated_line.push(text, span_style);
                    continue;
                }


                auto first = text.substr(0, marker.start());
                auto middle = text.substr(marker.start(), marker.size());
                auto end = text.substr(marker.end());
                annotated_line.push(first, span_style);

                auto mid_style = span_style;
                mid_style.underline_marker = "^";
                annotated_line.push(middle, mid_style);

                annotated_line.push(end, span_style);
            }

            annotated_line.build_indices();

            if (!annotated_line.empty()) {
                canvas.draw_text(annotated_line, x, y);
                y += bottom_padding;
            }
            (void)as;

            ++y;
            ++l;
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
