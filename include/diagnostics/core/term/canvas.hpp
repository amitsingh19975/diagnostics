#ifndef AMT_DARK_DIAGNOSTICS_CORE_TERM_CANVAS_HPP
#define AMT_DARK_DIAGNOSTICS_CORE_TERM_CANVAS_HPP

#include "../small_vec.hpp"
#include "annotated_string.hpp"
#include "color.hpp"
#include "terminal.hpp"
#include "../utf8.hpp"
#include "style.hpp"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace dark::term {

    struct LineCharSet {
        std::string_view vertical;
        std::string_view horizonal;
        std::string_view turn_right; // top left corner
        std::string_view turn_down; // top right corner
        std::string_view turn_left; // bottom right corner
        std::string_view turn_up; // bottom left corner
        std::string_view cross;
        std::string_view plus;
    };

    struct BoxCharSet {
        std::string_view vertical;
        std::string_view horizonal;
        std::string_view top_left;
        std::string_view top_right;
        std::string_view bottom_right;
        std::string_view bottom_left;
        std::string_view left_connector; // -|
        std::string_view top_connector;
        std::string_view right_connector; // |-
        std::string_view bottom_connector; // |-
    };

    struct ArrowCharSet {
        std::string_view up;
        std::string_view right;
        std::string_view down;
        std::string_view left;
    };

    namespace char_set {
        namespace line {
            static constexpr auto rounded = LineCharSet {
                .vertical = "│",
                .horizonal = "─",
                .turn_right = "╭",
                .turn_down = "╮",
                .turn_left = "╯",
                .turn_up = "╰",
                .cross = "╳",
                .plus = "┽"
            };

            static constexpr auto rounded_bold = LineCharSet {
                .vertical = "┃",
                .horizonal = "━",
                .turn_right = "┏",
                .turn_down = "┓",
                .turn_left = "┛",
                .turn_up = "┗",
                .cross = "╳",
                .plus = "╋"
            };

            static constexpr auto rect = LineCharSet {
                .vertical = "│",
                .horizonal = "─",
                .turn_right = "┌",
                .turn_down = "┐",
                .turn_left = "┘",
                .turn_up = "└",
                .cross = "x",
                .plus = "+"
            };

            static constexpr auto rect_bold = LineCharSet {
                .vertical = "┃",
                .horizonal = "━",
                .turn_right = "┏",
                .turn_down = "┓",
                .turn_left = "┛",
                .turn_up = "┗",
                .cross = "✖",
                .plus = "➕"
            };

            static constexpr auto dotted = LineCharSet {
                .vertical = "┆",
                .horizonal = "┄",
                .turn_right = "┌",
                .turn_down = "┐",
                .turn_left = "┘",
                .turn_up = "└",
                .cross = "x",
                .plus = "+"
            };
            static constexpr auto dotted_bold = LineCharSet {
                .vertical = "┇",
                .horizonal = "┉",
                .turn_right = "┏",
                .turn_down = "┓",
                .turn_left = "┛",
                .turn_up = "┗",
                .cross = "✖",
                .plus = "➕"
            };
        } // namespace line

        namespace box {
            static constexpr auto ascii = BoxCharSet {
                .vertical = "|",
                .horizonal = "-",
                .top_left = "",
                .top_right = "",
                .bottom_right = "",
                .bottom_left = "",
                .left_connector = "",
                .top_connector = "",
                .right_connector = "",
                .bottom_connector = "",
            };

            static constexpr auto rounded = BoxCharSet {
                .vertical = "│",
                .horizonal = "─",
                .top_left = "╭",
                .top_right = "╮",
                .bottom_right = "╯",
                .bottom_left = "╰",
                .left_connector = "┤",
                .top_connector = "┴",
                .right_connector = "├",
                .bottom_connector = "┬",
            };

            static constexpr auto rounded_bold = BoxCharSet {
                .vertical = "┃",
                .horizonal = "━",
                .top_left = "┏",
                .top_right = "┓",
                .bottom_right = "┛",
                .bottom_left = "┗",
                .left_connector = "┨",
                .top_connector = "┸",
                .right_connector = "┠",
                .bottom_connector = "┰",
            };

            static constexpr auto doubled = BoxCharSet {
                .vertical = "║",
                .horizonal = "═",
                .top_left = "╔",
                .top_right = "╗",
                .bottom_right = "╝",
                .bottom_left = "╚",
                .left_connector = "╢",
                .top_connector = "╧",
                .right_connector = "╟",
                .bottom_connector = "╤",
            };

            static constexpr auto dotted = BoxCharSet {
                .vertical = "┆",
                .horizonal = "┄",
                .top_left = "┌",
                .top_right = "┐",
                .bottom_right = "┘",
                .bottom_left = "└",
                .left_connector = "┤",
                .top_connector = "┴",
                .right_connector = "├",
                .bottom_connector = "┬",
            };
            static constexpr auto dotted_bold = BoxCharSet {
                .vertical = "┇",
                .horizonal = "┉",
                .top_left = "┏",
                .top_right = "┓",
                .bottom_right = "┛",
                .bottom_left = "┗",
                .left_connector = "┨",
                .top_connector = "┸",
                .right_connector = "┠",
                .bottom_connector = "┰",
            };
        } // namespace box

        namespace arrow {
            static constexpr auto ascii = ArrowCharSet {
                .up = "^",
                .right = ">",
                .down = "v",
                .left = "<"
            };
            static constexpr auto basic = ArrowCharSet {
                .up = "↑",
                .right = "→",
                .down = "↓",
                .left = "←"
            };

            static constexpr auto basic_bold = ArrowCharSet {
                .up = "⬆",
                .right = "\u{27A1}",
                .down = "⬇",
                .left = "⬅"
            };

            static constexpr auto doubled = ArrowCharSet {
                .up = "⇑",
                .right = "⇒",
                .down = "⇓",
                .left = "⇐"
            }; 
        } // namespace arrow
    } // namespace char_set

    struct BoundingBox {
        unsigned x;
        unsigned y;
        unsigned width;
        unsigned height;

        constexpr auto cx() const noexcept -> unsigned {
            return x + width / 2;
        }

        constexpr auto cy() const noexcept -> unsigned {
            return y + height / 2;
        }

        constexpr auto top_left() const noexcept -> std::pair<unsigned, unsigned> {
            return { x, y };
        }

        constexpr auto top_right() const noexcept -> std::pair<unsigned, unsigned> {
            return { x + width, y };
        }

        constexpr auto bottom_right() const noexcept -> std::pair<unsigned, unsigned> {
            return { x + width, y + height };
        }

        constexpr auto bottom_left() const noexcept -> std::pair<unsigned, unsigned> {
            return { x, y + height };
        }
    };

    struct Point {
        unsigned x;
        unsigned y;

        constexpr auto operator==(Point const&) const noexcept -> bool = default;

        constexpr auto to_pair() const noexcept -> std::pair<unsigned, unsigned> {
            return std::make_pair(x, y);
        }

        constexpr auto dx(Point o) const noexcept -> int {
            return int(x) - int(o.x);
        }

        constexpr auto dy(Point o) const noexcept -> int {
            return int(y) - int(o.y);
        }
    };

    struct Canvas {
        using size_type = std::size_t;
        static constexpr auto tab_width = 4ul;
        static constexpr size_type min_cols = 50;
        static constexpr size_type max_cols = 200;

        struct Cell {
            Style style {};
            char c[4] = {0, 0, 0, 0}; // utf-8
            std::uint8_t len{};

            constexpr auto write(std::string_view ch) noexcept -> size_type {
                if (ch.empty()) {
                    len = 0;
                    return 0;
                }
                len = static_cast<std::uint8_t>(std::min<size_type>(core::utf8::get_length(ch[0]), ch.size()));

                DARK_ASSUME(len < 5);

                for (auto i = 0ul; i < len; ++i) c[i] = ch[i];
                return size();
            }

            constexpr auto write(char ch) noexcept -> size_type {
                c[0] = ch;
                len = 1;
                return size();
            }

            constexpr auto to_string() const noexcept -> std::string_view {
                return std::string_view{ c, size() };
            }

            constexpr auto size() const noexcept -> size_type { return static_cast<size_type>(len); }
            constexpr auto empty() const noexcept -> bool { return size() == 0; }
        };

        Canvas(size_type cols)
            : m_rows(2)
            , m_cols(std::clamp(cols, min_cols, max_cols))
            , m_cells(m_rows * m_cols)
        {}

        auto render(Terminal& term) const noexcept -> void {
            auto const& self = *this;
            for (auto i = 0ul; i <= m_max_rows_written; ++i) {
                auto new_cols = 0ul;
                auto found_char{false};
                for (auto j = 0ul; j < cols(); ++j) {
                    if (!self(i, j).empty()) {
                        new_cols = j;
                        found_char = true;
                    }
                }
                if (!found_char) {
                    term.reset_color();
                }
                new_cols = std::min(cols(), new_cols + 1);

                for (auto j = 0ul; j < new_cols; ++j) {
                    auto const& cell = self(i, j);
                    term.change_color(
                        cell.style.text_color,
                        cell.style.bg_color,
                        {
                            .bold = cell.style.bold,
                            .dim = cell.style.dim,
                            .strike = cell.style.strike,
                            .italic = cell.style.italic
                        }
                    );
                    if (cell.empty()) {
                        term.write(" ");
                        continue;
                    }
                    term.write(cell.to_string());
                }
                term.write("\n");
            }

            term.reset_color();
        }

        constexpr auto operator()(size_type r, size_type c) const noexcept -> Cell const& {
            assert(r < rows() && "out of bound");
            assert(c < cols() && "out of bound");
            return m_cells[r * cols() + c];
        }

        constexpr auto operator()(size_type r, size_type c) noexcept -> Cell& {
            assert(r < rows() && "out of bound");
            assert(c < cols() && "out of bound");
            return m_cells[r * cols() + c];
        }

        constexpr auto rows() const noexcept -> size_type { return m_rows; }
        constexpr auto cols() const noexcept -> size_type { return m_cols; }

        auto add_rows(size_type rs = 1) -> void {
            m_rows += rs;
            m_cells.resize(m_rows * cols());
        }

        constexpr auto draw_pixel(
            dsize_t x,
            dsize_t y,
            std::string_view ch,
            Style style = {}
        ) noexcept {
            if (x >= cols()) return;
            while (y >= rows()) {
                add_rows(rows() * 2);
            }

            m_max_rows_written = std::max(m_max_rows_written, y);

            auto cell = Cell{
                .style = style
            };
            auto& current = this->operator()(y, x);
            if (current.style.zIndex > style.zIndex) return;
            cell.write(ch);
            current = cell;
        }

        // INFO: Draws rectangular paths; no diagonals
        // Ex: (2, 2)
        // |    |
        // |    |
        // |    |
        // |    |--------(12, 8)
        // |
        // -----------------
        constexpr auto draw_line(
            dsize_t x1,
            dsize_t y1,
            dsize_t x2,
            dsize_t y2,
            Style style = {},
            bool top_bias = false,
            LineCharSet normal_set = char_set::line::rounded,
            LineCharSet bold_set = char_set::line::rounded_bold
        ) noexcept -> BoundingBox {
            auto set = style.bold ? bold_set : normal_set;
            auto p1 = Point(x1, y1);
            auto p2 = Point(x2, y2);

            if (x1 > x2) {
                std::swap(p1, p2);
            }

            if (p1.y == p2.y) {
                // Case 1:
                // |
                // |  p1-------p2
                // |
                // |
                // |--------------
                for (auto x = p1.x; x <= p2.x; ++x) {
                    draw_pixel(
                        x,
                        p1.y,
                        set.horizonal
                    );
                }
            } else if (p1.y < p2.y) {
                // Case 2:
                // |
                // |  p1
                // |  |
                // |  |-------p2
                // |--------------
                // 
                // |
                // |  p1
                // |  |
                // |  p2
                // |--------------

                auto sy = std::min(p1.y, p2.y);
                auto ey = std::max(p1.y, p2.y);
                auto bx = top_bias ? p1.x : p2.x;
                for (auto y = sy; y <= ey; ++y) {
                    draw_pixel(
                        bx,
                        y,
                        set.vertical
                    );
                }

                if (p1.x != p2.x) {
                    auto sx = std::min(p1.x, p2.x);
                    auto ex = std::max(p1.x, p2.x);
                    auto by = top_bias ? p1.y : p2.y;
                    for (auto x = sx; x <= ex; ++x) {
                        draw_pixel(
                            x,
                            by,
                            set.horizonal
                        );
                    }
                    auto corner = top_bias ? set.turn_right : set.turn_left;
                    if (!corner.empty()) {
                        draw_pixel(
                            bx,
                            by,
                            corner
                        );
                    }
                }
            } else {
                // Case 3:
                // |          p2
                // |          |
                // |          |
                // |  p1------|
                // |--------------
                // |          p2
                // |          |
                // |          |
                // |          p1
                // |--------------
                auto sy = std::min(p1.y, p2.y);
                auto ey = std::max(p1.y, p2.y);

                auto bx = top_bias ? p1.x : p2.x;
                for (auto y = sy; y <= ey; ++y) {
                    draw_pixel(
                        bx,
                        y,
                        set.vertical
                    );
                }

                if (p1.x != p2.x) {
                    auto sx = std::min(p1.x, p2.x);
                    auto ex = std::max(p1.x, p2.x);
                    auto by = top_bias ? p2.y : p1.y;
                    for (auto x = sx; x <= ex; ++x) {
                        draw_pixel(
                            x,
                            by,
                            set.horizonal
                        );
                    }

                    auto corner = top_bias ? set.turn_right : set.turn_left;
                    if (!corner.empty()) {
                        draw_pixel(
                            bx,
                            by,
                            corner
                        );
                    }
                }
            }

            return {
                .x = std::min(x1, x2),
                .y = std::min(y1, y2),
                .width = std::max(x1, x2),
                .height = std::max(y1, y2),
            };
        }

        constexpr auto draw_path(
            std::span<Point> path,
            Style style = {},
            LineCharSet normal_set = char_set::line::rounded,
            LineCharSet bold_set = char_set::line::rounded_bold
        ) noexcept -> BoundingBox {
            auto set = style.bold ? bold_set : normal_set;
            if (path.empty()) return {};

            if (path.size() == 1) {
                auto el = path[0];
                draw_pixel(el.x, el.y, set.cross);
                return {
                    .x = el.x,
                    .y = el.y,
                    .width = 1,
                    .height = 1,
                };
            } else if (path.size() == 2) {
                auto p0 = path[0]; 
                auto p1 = path[1]; 
                return draw_line(
                    p0.x, p0.y,
                    p1.x, p1.y,
                    style,
                    false,
                    normal_set,
                    bold_set
                );
            }

            constexpr auto norm = [](int dx, int dy) {
                return std::pair<int, int>{
                    (dx == 0) ? 0 : dx / std::abs(dx),
                    (dy == 0) ? 0 : dy / std::abs(dy)
                };
            };

            // Replace corners with previous point to avoid sharp turns
            auto size = path.size();
            core::SmallVec<std::pair<Point, std::string_view>, 32> corners;

            auto helper = [this, norm, set, &corners, style, normal_set, bold_set](
                Point p0,
                Point p1,
                Point p2
            ) {
                auto dx0 = int(p1.x) - int(p0.x);
                auto dy0 = int(p1.y) - int(p0.y);

                auto dx1 = int(p2.x) - int(p1.x);
                auto dy1 = int(p2.y) - int(p1.y);
                auto d0 = norm(dx0, dy0);
                auto d1 = norm(dx1, dy1);

                auto dx = int(p2.x) - int(p0.x);
                auto dy = int(p2.y) - int(p0.y);

                std::string_view corner;
                bool top_bias = false;

                if (dx * dy < 0) {
                    //  Case 1:
                    //   p1------p0
                    //   |      /
                    //   |     /
                    //   |    /
                    //   |   /
                    //   |  / slop m
                    //   p2---------
                    //  Case 2:
                    //           p0
                    //          / |
                    //         /  |
                    //        /   |
                    //       /    |
                    //      /     |
                    //   p2------p1

                    // top-left and bottom right corners
                    auto np = Point( std::min(p0.x, p2.x), std::min(p0.y, p2.y) );
                    if (np == p1) {
                        // top-right
                        corner = set.turn_right;
                        top_bias = true;
                    } else {
                        // bottom-left
                        corner = set.turn_left;
                    }
                } else {
                    auto np = Point( std::min(p0.x, p2.x), std::max(p0.y, p2.y) );
                    if (np == p1) {
                        // bottom-right
                        corner = set.turn_up;
                    } else {
                        // top-left
                        corner = set.turn_down;
                        top_bias = true;
                    }
                }

                if (d0 != d1 && !corner.empty()) {
                    corners.emplace_back(p1, corner);
                }

                draw_line(
                    p0.x, p0.y,
                    p1.x, p1.y,
                    style,
                    top_bias,
                    normal_set,
                    bold_set
                );
            };


            for (auto i = 0ul; i < size - 2; ++i) {
                helper(
                    path[i],
                    path[i + 1],
                    path[i + 2]
                ); 
            }

            auto p0 = path[size - 2];
            auto p1 = path[size - 1];

            draw_line(
                p0.x, p0.y,
                p1.x, p1.y,
                style
            );

            if (p1 == path[0]) {
                helper(
                    p0, p1, path[1]
                );
            }

            for (auto [p, line]: corners) {
                draw_pixel(
                    p.x, p.y,
                    line,
                    style
                );
            }

            return {};
        }

        constexpr auto draw_box(
            dsize_t x,
            dsize_t y,
            dsize_t width,
            dsize_t height,
            Style style = {},
            BoxCharSet normal_set = char_set::box::rounded,
            BoxCharSet bold_set = char_set::box::rounded_bold
        ) noexcept -> BoundingBox {
            auto set = style.bold ? bold_set : normal_set;
            auto x1 = x;
            auto y1 = y;
            auto x2 = x + width;
            auto y2 = y + height;

            // 1. Draw top part
            {
                for (auto s = x1; s <= x2; ++s) {
                    draw_pixel(
                        s,
                        y1,
                        set.horizonal,
                        style
                    );
                }
            }

            // 2. Draw right part
            {
                for (auto s = y1; s <= y2; ++s) {
                    draw_pixel(
                        x2,
                        s,
                        set.vertical,
                        style
                    );
                }
            }

            // 3. Draw bottom part
            {
                for (auto s = x1; s <= x2; ++s) {
                    draw_pixel(
                        s,
                        y2,
                        set.horizonal,
                        style
                    );
                }
            }

            // 4. Draw left part
            {
                for (auto s = y1; s <= y2; ++s) {
                    draw_pixel(
                        x1,
                        s,
                        set.vertical,
                        style
                    );
                }
            }

            // 5. Draw corners
            std::array corners {
                std::make_pair(Point(x1, y1), set.top_left),
                std::make_pair(Point(x2, y1), set.top_right),
                std::make_pair(Point(x2, y2), set.bottom_right),
                std::make_pair(Point(x1, y2), set.bottom_left)
            };

            for (auto [p, c]: corners) {
                if (c.empty()) continue;
                draw_pixel(
                    p.x, p.y,
                    c,
                    style
                );
            }

            return {
                .x = x,
                .y = y,
                .width = width,
                .height = height
            };
        }

        struct TextRenderResult {
            BoundingBox bbox;
            std::size_t left{};
        };

        auto draw_text(
            std::string_view text,
            dsize_t x,
            dsize_t y,
            TextStyle style = {}
        ) noexcept -> TextRenderResult {
            auto as = AnnotatedString{};
            as.push(text);
            return draw_text(std::move(as), x, y, style);
        }

        constexpr auto draw_text(
            AnnotatedString as,
            dsize_t x,
            dsize_t y,
            TextStyle style = {}
        ) noexcept -> TextRenderResult {
            as.build_indices();

            auto padding = style.padding;
            auto max_space = std::min<size_type>(
                style.max_width,
                cols() - std::min<size_type>(x, cols())
            ); 
            // |....[----space----]|

            auto container = BoundingBox {
                .x = x + padding.left,
                .y = y + padding.top,
                .width = std::max(static_cast<dsize_t>(max_space), padding.right) - padding.right,
                .height = 0
            };
            max_space = container.width;

            auto bbox = BoundingBox(x, y, 0, 0);
            if (max_space == 0) return { bbox, as.size() };

            x += padding.left;
            y += padding.top;

            auto max_x = x;

            if (!style.word_wrap) {
                auto size = std::min(as.size(), max_space);
                style.max_lines = 1;
                auto [consumed, bottom_padding] = draw_text_helper(
                    as,
                    x,
                    y,
                    size,
                    container,
                    style
                );

                y += std::max(bottom_padding, padding.bottom) + 1;

                return {
                    .bbox = BoundingBox(
                        bbox.x,
                        bbox.y,
                        std::min(x, static_cast<dsize_t>(cols() - 1)),
                        y
                    ),
                    .left = as.size() - consumed
                };
            }
            auto current_line = 0u;
            while (!as.empty()) {
                current_line += 1;

                max_x = std::max(max_x, x);
                x = container.x;

                auto old_size = as.size();
                auto [consumed, bottom_padding] = draw_text_helper(
                    as,
                    x, y,
                    max_space,
                    container,
                    style,
                    current_line
                );
                as.shift(consumed);
                padding.bottom = std::max(padding.bottom, bottom_padding);

                // Ensures no infinite loop
                if (old_size == as.size()) break;
                y += 1;
                if (current_line == style.max_lines) break;
            }
            x = std::min(std::max(max_x, x), container.width) + padding.right;
            y += padding.bottom;

            return { BoundingBox(bbox.x, bbox.y, x, y), as.size() };
        }

        constexpr auto draw_boxed_text(
            std::string_view text,
            dsize_t x,
            dsize_t y,
            TextStyle style = {},
            BoxCharSet normal_set = char_set::box::rounded,
            BoxCharSet bold_set = char_set::box::rounded_bold
        ) noexcept -> TextRenderResult {
            auto as = AnnotatedString();
            as.push(text);
            return draw_boxed_text(as, x, y, style, normal_set, bold_set);
        }

        constexpr auto draw_boxed_text(
            AnnotatedString as,
            dsize_t x,
            dsize_t y,
            TextStyle style = {},
            BoxCharSet normal_set = char_set::box::rounded,
            BoxCharSet bold_set = char_set::box::rounded_bold
        ) noexcept -> TextRenderResult {
            style.max_width = std::min(dsize_t(cols() - 2), style.max_width);
            auto [bbox, left] = draw_text(
                std::move(as),
                x + 1, y + 1,
                style
            );
            return {
                draw_box(x, y, bbox.width, bbox.height, style.to_style(), normal_set, bold_set),
                left
            };
        }

        constexpr auto draw_marked_text(
            std::string_view text,
            std::string_view marker,
            dsize_t x,
            dsize_t y,
            TextStyle style = {},
            Color marker_color = Color::Default
        ) noexcept -> TextRenderResult {
            if (marker_color == Color::Default) {
                marker_color = style.text_color;
            }

            auto [bbox, left] = draw_text(text, x, y);
            auto tmp = core::utf8::PackedUTF8(marker);
            style.text_color = marker_color;

            dsize_t step = std::min(static_cast<dsize_t>(tmp.size()), bbox.width);

            dsize_t tmp_x = bbox.x;
            for (auto k = 0u; k < bbox.width; k += step) {
                auto kb = std::min(step, bbox.width - k);
                for (auto i = 0ul; i < kb; ++i) {
                    auto m = tmp[i];
                    draw_pixel(tmp_x, bbox.y + 1, m, style.to_style());
                }
                tmp_x += kb;
            }
            bbox.height += 1;
            return { bbox, left };
        }
    private:
        constexpr auto draw_text_helper(
            AnnotatedString& as,
            dsize_t& x,
            dsize_t y,
            size_type size,
            BoundingBox container,
            TextStyle style = {},
            dsize_t current_line = 1,
            size_type max_as_size = std::numeric_limits<size_type>::max()
        ) noexcept -> std::pair<std::size_t, unsigned> {
            auto tmp_x = x;
            auto total_consumed = 0ul;
            std::tuple<std::string_view, SpanStyle, std::string_view> temp_buff[max_cols + 5] = {};
            auto start_x = x;
            unsigned bottom_padding = 0u;
            unsigned top_padding = 0u;
            auto text_size = std::min(as.size(), max_as_size);

            auto helper = [
                &total_consumed,
                &as, &tmp_x, x,
                size, &temp_buff,
                &bottom_padding,
                &top_padding,
                current_line,
                style
            ](
                size_type start, size_type end
            ) { 
                struct State {
                    size_type start;
                    size_type total_consumed;
                    unsigned x;
                    unsigned bottom_padding;
                    unsigned top_padding;
                    SpanStyle word_style;
                };
                auto stored_state = State {
                    .start = start,
                    .total_consumed = total_consumed,
                    .x = tmp_x,
                    .bottom_padding = bottom_padding,
                    .top_padding = top_padding,
                    .word_style = {}
                };

                if (style.trim_prefix) {
                    for (; start < end; ++start, ++total_consumed) {
                        auto [ch, sp_style, marker] = as[start];
                        (void)marker;
                        if (sp_style.padding) {
                            auto st = sp_style.padding.value_or(PaddingValues());
                            st.left = 0;
                            as.update_padding(start, st);
                        }
                        if (!std::isspace(ch[0])) break;
                    }
                }

                for (; start < end && (tmp_x < size); ++start, ++total_consumed) {
                    auto [ch, span_style, marker] = as[start];
                    if (as.is_word_end(start)) {
                        stored_state = State {
                            .start = start,
                            .total_consumed = total_consumed,
                            .x = tmp_x,
                            .bottom_padding = bottom_padding,
                            .top_padding = top_padding,
                            .word_style = {}
                        };
                    }
                    auto padding = span_style.padding.value_or(PaddingValues());
                    tmp_x += padding.left;

                    auto np = padding;
                    np.left = 0;
                    as.update_padding(start, np);

                    if (tmp_x + padding.right >= size + x) {
                        break;
                    }

                    bottom_padding = std::max(
                        padding.bottom,
                        bottom_padding
                    );
                    top_padding = std::max(
                        padding.top,
                        top_padding
                    );

                    if (ch[0] == '\n') {
                        ++total_consumed;
                        break;
                    } else if (ch[0] == '\t') {
                        for (auto i = 0ul; i < tab_width; ++i) {
                            temp_buff[tmp_x++] = { " ", span_style, marker };
                        }
                        continue;
                    }
                    temp_buff[tmp_x++] = { ch, span_style, marker };
                    tmp_x += padding.right;
                    np.right = 0;
                    as.update_padding(start, np);
                }

                if (style.break_whitespace && !as.is_word_end(start)) {
                    start = stored_state.start;
                    total_consumed = stored_state.total_consumed;
                    tmp_x = stored_state.x;
                    bottom_padding = stored_state.bottom_padding;
                    top_padding = stored_state.top_padding;
                    for (auto i = tmp_x; i < size; ++i) temp_buff[i] = {};
                }

                if (current_line == 1) {
                    top_padding = std::max(top_padding, style.padding.top) - style.padding.top;
                }

                if (current_line == style.max_lines || (as.size() - total_consumed) == 0) {
                    bottom_padding = std::max(bottom_padding, style.padding.bottom);
                }
            };

            if (current_line < style.max_lines) {
                helper(0, text_size);
            } else {
                switch (style.overflow) {
                case TextOverflow::none: {
                    helper(0, text_size);
                } break;
                case TextOverflow::ellipsis: {
                    helper(0, text_size);

                    auto const buff_size = (tmp_x - start_x);
                    auto dots = std::min(buff_size, 3u);
                    for (auto i = 0ul; i < dots; ++i) {
                        temp_buff[tmp_x - 1 - i] = { ".", {}, {} };
                    }
                } break;
                case TextOverflow::middle_ellipsis: {
                    auto text_len = std::min(text_size, max_cols);
                    auto middle = text_len / 2;
                    auto mid_col = size / 2;
                    auto s0 = 0ul;
                    auto e0 = std::min(std::max(middle, 2ul) - 2, mid_col);
                    auto s1 = std::max(mid_col, text_len) - mid_col - 2;
                    auto e1 = text_len;
                    helper(s0, e0);
                    tmp_x += 5;
                    helper(s1, e1);

                    temp_buff[e0] = { " ", {}, {} };
                    temp_buff[e0 + 1] = { ".", {}, {} };
                    temp_buff[e0 + 2] = { ".", {}, {} };
                    temp_buff[e0 + 3] = { ".", {}, {} };
                    temp_buff[s1] = { " ", {}, {} };
                } break;
                case TextOverflow::start_ellipsis: {
                    auto total_len = text_size;
                    auto mid_col = std::max(size, size_type{3}) - 3;
                    auto s0 = std::max(mid_col, total_len) - mid_col;
                    auto e0 = total_len;
                    temp_buff[tmp_x++] = { ".", {}, {} };
                    temp_buff[tmp_x++] = { ".", {}, {} };
                    temp_buff[tmp_x++] = { ".", {}, {} };
                    helper(s0, e0);
                } break;
                }
            }

            y += top_padding;

            auto start_offset = 0u;
            auto space_left = (std::max(static_cast<dsize_t>(container.width), tmp_x) - tmp_x);
            if (style.align == TextAlign::Center) {
                start_offset += space_left / 2;
            } else if (style.align == TextAlign::Right) {
                start_offset += space_left;
            }

            for (; start_x < tmp_x; ++start_x) {
                auto [ch, st, marker] = temp_buff[start_x];
                draw_pixel(
                    start_x + start_offset,
                    y,
                    ch,
                    st.to_style(style)
                );
                if (!marker.empty()) {
                    draw_pixel(
                        start_x,
                        y + 1,
                        marker,
                        st.to_style(style)
                    );
                }
            }

            x = tmp_x + start_offset;
            return { total_consumed, bottom_padding + top_padding };
        }

    private:
        size_type m_rows{};
        size_type m_cols{};
        std::vector<Cell> m_cells;
        dsize_t m_max_rows_written{};
    };

} // namespace dark::term

#endif // AMT_DARK_DIAGNOSTICS_CORE_TERM_CANVAS_HPP
