#ifndef AMT_DARK_DIAGNOSTICS_CORE_TERM_CANVAS_HPP
#define AMT_DARK_DIAGNOSTICS_CORE_TERM_CANVAS_HPP

#include "../small_vec.hpp"
#include "annotated_string.hpp"
#include "color.hpp"
#include "../../core/cow_string.hpp"
#include "../../span.hpp"
#include "terminal.hpp"
#include "../utf8.hpp"
#include "style.hpp"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <string_view>
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
                .up = "△",
                .right = "▷",
                .down = "▽",
                .left = "◁"
            };

            static constexpr auto basic_bold = ArrowCharSet {
                .up = "▲",
                .right = "▶",
                .down = "▼",
                .left = "◀"
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

        constexpr auto operator==(BoundingBox const&) const noexcept -> bool = default;
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
            if (current.style.z_index > style.z_index) return;
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
                        set.horizonal,
                        style
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
                        set.vertical,
                        style
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
                            set.horizonal,
                            style
                        );
                    }
                    auto corner = top_bias ? set.turn_right : set.turn_left;
                    if (!corner.empty()) {
                        draw_pixel(
                            bx,
                            by,
                            corner,
                            style
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
                        set.vertical,
                        style
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
                            set.horizonal,
                            style
                        );
                    }

                    auto corner = top_bias ? set.turn_right : set.turn_left;
                    if (!corner.empty()) {
                        draw_pixel(
                            bx,
                            by,
                            corner,
                            style
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

            if (!style.bg_color.is_invalid()) {
                for (auto r = 0u; r < height; r++) {
                    for (auto c = 0u; c < width; ++c) {
                        if (y + r >= rows() || x + c >= cols()) continue;
                        auto& current = this->operator()(y + r, x + c);
                        if (current.empty()) {
                            draw_pixel(
                                x + c,
                                y + r,
                                " ",
                                style
                            );
                        }
                    }
                }
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

        template <bool ShouldDraw = true>
        constexpr auto draw_text(
            AnnotatedString const& as,
            dsize_t x,
            dsize_t y,
            TextStyle style = {}
        ) noexcept -> TextRenderResult {
            auto padding = style.padding;
            auto max_space = std::min<size_type>(
                style.max_width,
                cols() - std::min<size_type>(x, cols())
            ); 
            // |....[----space----]|

            auto container = BoundingBox {
                .x = x,
                .y = y,
                .width = static_cast<dsize_t>(max_space),
                .height = 0
            };

            auto bbox = BoundingBox(x, y, 0, 0);
            if (max_space == 0) return { bbox, as.size() };

            x += padding.left;
            y += padding.top;

            auto max_x = x;

            if (!style.word_wrap) {
                style.max_lines = 1;
                [[maybe_unused]] auto [
                    chunk_start,
                    text_start,
                    bottom_padding,
                    consumed
                ] = draw_text_helper<ShouldDraw>(
                    as,
                    0,
                    0,
                    x,
                    y,
                    container,
                    style
                );

                y += std::max(bottom_padding, padding.bottom) + 1;
                auto width = std::min(x, static_cast<dsize_t>(cols() - 1)) - bbox.x;
                return {
                    .bbox = BoundingBox(
                        bbox.x,
                        bbox.y,
                        std::min(width + padding.right, container.width),
                        y - bbox.y
                    ),
                    .left = as.size() - consumed
                };
            }
            auto current_line = 0u;
            auto old_chunk_start = std::size_t{};
            auto old_text_start = std::size_t{};
            auto total_consumed = std::size_t{};
            auto total_size = std::size_t{};

            for (auto const& el: as.strings) {
                total_size += el.first.size();
            }

            auto is_first{true};
            while (total_consumed < total_size) {
                current_line += 1;

                max_x = std::max(max_x, x);
                x = container.x + padding.left;
                if (!is_first) x += style.word_wrap_start_padding;

                auto [
                    chunk_start,
                    text_start,
                    bottom_padding,
                    consumed
                ] = draw_text_helper<ShouldDraw>(
                    as,
                    old_chunk_start,
                    old_text_start,
                    x, y,
                    container,
                    style,
                    current_line
                );
                old_chunk_start = chunk_start;
                old_text_start = text_start;
                total_consumed += consumed;

                // Ensures no infinite loop
                if (consumed == 0) break;
                y += bottom_padding + 1;
                if (current_line == style.max_lines) break;
                is_first = false;
            }
            x = std::min(std::max(max_x, x), container.bottom_right().first) + padding.right;
            y += padding.bottom;

            return {
                BoundingBox(bbox.x, bbox.y, x - bbox.x, y - bbox.y),
                as.size()
            };
        }

        auto measure_text(
            AnnotatedString const& s,
            dsize_t x,
            dsize_t y,
            TextStyle style = {}
        ) noexcept -> BoundingBox {
            auto tmp = draw_text<false>(s, x, y, style);
            return tmp.bbox;
        }

        auto draw_boxed_text(
            std::string_view text,
            dsize_t x,
            dsize_t y,
            TextStyle style = {},
            BoxCharSet normal_set = char_set::box::rounded,
            BoxCharSet bold_set = char_set::box::rounded_bold
        ) noexcept -> TextRenderResult {
            return draw_boxed_text(
                AnnotatedString::builder()
                    .push(text)
                    .build(),
                x, y,
                style,
                normal_set,
                bold_set
            );
        }

        constexpr auto draw_boxed_text(
            AnnotatedString const& text,
            dsize_t x,
            dsize_t y,
            TextStyle style = {},
            BoxCharSet normal_set = char_set::box::rounded,
            BoxCharSet bold_set = char_set::box::rounded_bold
        ) noexcept -> TextRenderResult {
            style.max_width = std::min(dsize_t(cols() - 2), style.max_width);
            auto [bbox, left] = draw_text(
                std::move(text),
                x + 1, y + 1,
                style
            );
            auto box = draw_box(x, y, bbox.width + 1, bbox.height + 1, style.to_style(), normal_set, bold_set);
            return {
                box,
                left
            };
        }

        constexpr auto draw_boxed_text_with_header(
            AnnotatedString const& header,
            AnnotatedString const& text,
            dsize_t x,
            dsize_t y,
            TextStyle body_style = {},
            TextStyle header_style = TextStyle::default_header_style(),
            BoxCharSet normal_set = char_set::box::rounded,
            BoxCharSet bold_set = char_set::box::rounded_bold
        ) noexcept -> TextRenderResult {
            auto [box, result] = draw_boxed_text(
                std::move(text),
                x, y,
                std::move(body_style),
                normal_set,
                bold_set
            );

            if (!header.strings.empty()) {
                if (header_style.text_color == Color::Default) {
                    header_style.text_color = body_style.text_color;
                }
                if (header_style.bg_color == Color::Default) {
                    header_style.bg_color = body_style.bg_color;
                }

                header_style.max_width = std::min(
                    header_style.max_width,
                    box.width - 1
                );

                draw_text(
                    std::move(header),
                    box.x + 1,
                    box.y,
                    std::move(header_style)
                );
            }

            return { box, result };
        }

        auto draw_boxed_text_with_header(
            std::string_view header,
            std::string_view text,
            dsize_t x,
            dsize_t y,
            TextStyle body_style = {},
            TextStyle header_style = TextStyle::default_header_style(),
            BoxCharSet normal_set = char_set::box::rounded,
            BoxCharSet bold_set = char_set::box::rounded_bold
        ) noexcept -> TextRenderResult {
            return draw_boxed_text_with_header(
                AnnotatedString::builder().push(header).build(),
                AnnotatedString::builder().push(text).build(),
                x, y,
                std::move(body_style),
                std::move(header_style),
                normal_set,
                bold_set
            );
        }

        auto draw_boxed_text_with_header(
            std::string_view header,
            AnnotatedString const& text,
            dsize_t x,
            dsize_t y,
            TextStyle body_style = {},
            TextStyle header_style = TextStyle::default_header_style(),
            BoxCharSet normal_set = char_set::box::rounded,
            BoxCharSet bold_set = char_set::box::rounded_bold
        ) noexcept -> TextRenderResult {
            return draw_boxed_text_with_header(
                AnnotatedString::builder().push(header).build(),
                std::move(text),
                x, y,
                std::move(body_style),
                std::move(header_style),
                normal_set,
                bold_set
            );
        }

        auto draw_boxed_text_with_header(
            AnnotatedString const& header,
            std::string_view text,
            dsize_t x,
            dsize_t y,
            TextStyle body_style = {},
            TextStyle header_style = TextStyle::default_header_style(),
            BoxCharSet normal_set = char_set::box::rounded,
            BoxCharSet bold_set = char_set::box::rounded_bold
        ) noexcept -> TextRenderResult {
            return draw_boxed_text_with_header(
                std::move(header),
                AnnotatedString::builder().push(text).build(),
                x, y,
                std::move(body_style),
                std::move(header_style),
                normal_set,
                bold_set
            );
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
        struct MeasureTextResult {
            unsigned cols_occupied{};
            bool can_overflow{false};
            unsigned global_size{};
            Span start_overflow{};
            Span middle_overflow{};
            std::size_t word_boundary; // global index to word end before cutting off
            // Minimum line boundry that would fit the line.
            std::pair<std::size_t, std::size_t> min_line_boundary;
        };

        constexpr auto measure_single_line_text_width_helper(
            AnnotatedString const& as,
            std::size_t chunk_start,
            std::size_t text_start,
            BoundingBox container,
            TextStyle const& style,
            unsigned total_occupied_cols = {}
        ) noexcept -> MeasureTextResult {
            auto const& strs = as.strings;
            if (strs.empty()) return {};

            auto res = MeasureTextResult();

            res.cols_occupied += style.padding.horizontal();
            auto max_width = std::max(container.width, style.padding.right) - style.padding.right;

            // auto mid = total_occupied_cols / 2;
            auto left_size = (std::max(max_width, dsize_t{3}) - 3) / 2;

            auto overflow_portion = std::max(total_occupied_cols, max_width) - max_width;
            auto middle_overflow_start = dsize_t{};

            for (auto i = chunk_start; i < strs.size(); ++i) {
                auto const& el = strs[i];
                auto text = el.first.to_borrowed();
                auto span = el.second;
                auto padding = span.padding.value_or(PaddingValues());
                res.cols_occupied += padding.left;
                if (total_occupied_cols != 0) {
                    if (res.start_overflow.empty()) {
                        if (total_occupied_cols < max_width + res.cols_occupied) {
                            res.start_overflow = Span(0, res.global_size);
                        }
                    }
                    if (res.middle_overflow.empty()) {
                        if (res.cols_occupied >= left_size && middle_overflow_start == 0) {
                            middle_overflow_start = res.global_size;
                        }
                        if (res.cols_occupied >= overflow_portion + left_size) {
                            res.middle_overflow = Span(middle_overflow_start, res.global_size);
                        }
                    }
                }

                bool is_between_word = !std::isspace(text[text_start]);
                auto old_cols = res.cols_occupied;
                while (text_start < text.size()) {
                    auto len = core::utf8::get_length(text[text_start]);
                    auto inc = text[text_start] == '\t' ? tab_width : 1;

                    if (text[text_start] == '\n') {
                        // last character before the newline ('\n' not included)
                        res.min_line_boundary = { i + 1, text_start + len };
                        res.word_boundary = res.global_size + len;
                        return res;
                    }
                    if (res.cols_occupied + inc <= max_width) {
                        auto old_is_between_word = is_between_word;
                        if (text_start + len < text.size()) {
                            is_between_word = !std::isspace(text[text_start + len]);

                            if (is_between_word != old_is_between_word) {
                                res.word_boundary = res.global_size + len;
                            }
                        }
                    } else {
                        res.can_overflow = true;
                    }

                    res.cols_occupied += inc;
                    if (total_occupied_cols != 0) {
                        if (res.start_overflow.empty()) {
                            if (total_occupied_cols < max_width + res.cols_occupied) {
                                res.start_overflow = Span(0, res.global_size);
                            }
                        }
                        if (res.middle_overflow.empty()) {
                            if (res.cols_occupied >= left_size && middle_overflow_start == 0) {
                                middle_overflow_start = res.global_size;
                            }
                            if (res.cols_occupied >= overflow_portion + left_size) {
                                res.middle_overflow = Span(middle_overflow_start, res.global_size);
                            }
                        }
                    }
                    text_start += len;
                    res.global_size += len;
                }

                if (res.cols_occupied - old_cols == 0) {
                    res.cols_occupied -= padding.left;
                } else {
                    res.cols_occupied += padding.right;
                }

                if (total_occupied_cols != 0) {
                    if (res.start_overflow.empty()) {
                        if (total_occupied_cols < max_width + res.cols_occupied) {
                            res.start_overflow = Span(0, res.global_size);
                        }
                    }
                    if (res.middle_overflow.empty()) {
                        if (res.cols_occupied >= left_size && middle_overflow_start == 0) {
                            middle_overflow_start = res.global_size;
                        }
                        if (res.cols_occupied >= overflow_portion + left_size) {
                            res.middle_overflow = Span(middle_overflow_start, res.global_size);
                        }
                    }
                }
                text_start = 0;
            }

            if (res.cols_occupied <= max_width) {
                res.word_boundary = std::string_view::npos;
            }
            return res;
        }

        template <bool ShouldDraw = true>
        auto draw_text_helper(
            AnnotatedString const& as,
            std::size_t chunk_start,
            std::size_t text_start,
            dsize_t& x,
            dsize_t y,
            BoundingBox container,
            TextStyle style = {},
            dsize_t current_line = 1
        ) noexcept -> std::tuple<std::size_t, std::size_t, unsigned, std::size_t> {
            if (current_line > style.max_lines) return {
                chunk_start, text_start, 0, 0
            };

            // remove spaces from space from the start. 
            if (style.trim_space) {
                auto const& strs = as.strings;
                for (; chunk_start < strs.size(); ++chunk_start) {
                    auto const& el = strs[chunk_start];
                    auto text = el.first.to_borrowed();
                    bool found_none_whitespace{false};
                    for (; text_start < text.size(); ++text_start) {
                        if (text[text_start] != ' ') {
                            found_none_whitespace = true;
                            break;
                        }
                    }
                    if (found_none_whitespace) break;
                }
            }

            auto width_info = measure_single_line_text_width_helper(
                as,
                chunk_start,
                text_start,
                container,
                style
            );

            unsigned bottom_padding = 0, top_padding = 0;
            auto max_x = container.bottom_right().first;
            max_x = std::max(max_x, style.padding.right) - style.padding.right;

            auto render = [
                this,
                &as, &x, y_start = y,
                &bottom_padding,
                &top_padding,
                style,
                max_x,
                current_line
            ](
                std::size_t& chunk_start,
                std::size_t& text_start,
                Span overflow_section,
                unsigned cols_occupied,
                std::size_t chunk_end = std::string_view::npos,
                std::size_t text_end = std::string_view::npos
            ) -> std::pair<bool, std::size_t> {
                 // compiler complains about the no being used even though we're are using it.
                (void)this;
                auto y = y_start;
                auto global_index = dsize_t{};
                auto needs_underline{false};
                std::size_t consumed = std::size_t{};
                chunk_end = std::min(as.strings.size(), chunk_end);
                if (chunk_start >= chunk_end) return { false, 0 };

                // get how many cells are empty so we can apply alignment based on that.
                auto free_space = std::max(max_x, cols_occupied) - cols_occupied;

                if (style.align == TextAlign::Center) {
                    x += free_space / 2;
                } else if (style.align == TextAlign::Right) {
                    x += free_space;
                }

                auto first_style = as.strings[chunk_start].second;
                auto start_x = x + first_style.padding.value_or(PaddingValues()).left;

                // Calculate the top padding so we apply before rendering.
                for (auto i = chunk_start; i < chunk_end; ++i) {
                    auto const& el = as.strings[i];
                    auto text = el.first.to_borrowed();
                    auto len = static_cast<dsize_t>(core::utf8::calculate_size(text));

                    auto span = Span::from_size(global_index, len);

                    if (overflow_section.force_merge(span) != overflow_section) {
                        auto padding = el.second.padding.value_or(PaddingValues());
                        top_padding = std::max(top_padding, padding.top);
                    }
                    global_index += len;
                }

                // If it's a first line and global style applies its own padding,
                // we merge the two padding just (CSS's margin behaviour).
                if (current_line == 1) {
                    top_padding = std::max(top_padding, style.padding.top) - style.padding.top;
                }

                y += top_padding;

                global_index = 0;
                auto ellipsis_rendered{false};

                for (; chunk_start < chunk_end; ++chunk_start) {
                    auto const& el = as.strings[chunk_start];
                    auto text = el.first.to_borrowed();
                    auto span_style = el.second;
                    if (max_x <= x) break;

                    auto padding = span_style.padding.value_or(PaddingValues());
                    x += padding.left;

                    auto old_x = x;
                    needs_underline |= !span_style.underline_marker.empty();

                    auto marker_index = std::size_t{};
                    // Render start ellipsis if the max line reached.
                    if (!ellipsis_rendered) {
                        if (style.max_lines == current_line && cols_occupied > max_x) {
                            if (style.overflow == TextOverflow::start_ellipsis) {
                                auto st = first_style.to_style(style);
                                for (auto i = 0ul; i < 3 && x < max_x; ++i) {
                                    if constexpr (ShouldDraw) {
                                        draw_pixel(x++, y, ".", st);
                                    }
                                }
                                ellipsis_rendered = true;
                            }
                        }
                    }

                    auto found_end = false;
                    while (text_start < text.size() && x < max_x) {
                        auto len = core::utf8::get_length(text[text_start]);
                        consumed += len;
                        if (global_index >= text_end) {
                            found_end = true;
                            break;
                        }

                        // checks if the range lies within the overflow range, which will be skipped.
                        auto can_skip = overflow_section.is_between(global_index);

                        auto txt = text.substr(text_start, len);
                        text_start += len;
                        if (txt[0] == '\n') {
                            found_end = true;
                            break;
                        }

                        global_index += len;

                        if (can_skip) {
                            if (!ellipsis_rendered) {
                                if (style.max_lines == current_line && cols_occupied > max_x) {
                                    if (style.overflow == TextOverflow::middle_ellipsis) {
                                        auto st = first_style.to_style(style);
                                        for (auto i = 0ul; i < 3 && x < max_x; ++i) {
                                            if constexpr (ShouldDraw) {
                                                draw_pixel(x++, y, ".", st);
                                            }
                                        }
                                        ellipsis_rendered = true;
                                    }
                                }
                            }
                        } else {
                            auto iter = 1ul;
                            if (txt[0] == '\t') {
                                iter = tab_width;
                                txt = " ";
                            } else if (text[text_start] == '\n') {
                                return { needs_underline, consumed };
                            }

                            for (auto i = 0ul; i < iter && x < max_x; ++i, ++x) {
                                if constexpr (ShouldDraw) {
                                    draw_pixel(x, y, txt, span_style.to_style(style));
                                    if (span_style.underline_marker.empty()) continue;
                                    auto mlen = core::utf8::get_length(text[marker_index]);
                                    draw_pixel(x, y + 1, span_style.underline_marker.substr(marker_index, mlen), span_style.to_style(style));
                                }
                            }
                        }

                        // iterate over the marker to get the next character.
                        if (!span_style.underline_marker.empty()) {
                            auto mlen = core::utf8::get_length(text[marker_index]);
                            marker_index = (marker_index + mlen) % span_style.underline_marker.size();
                        }
                    }

                    if (x - old_x == 0) {
                        x -= padding.left;
                    } else {
                        x += padding.right;
                        bottom_padding = std::max(bottom_padding, padding.bottom);
                        top_padding = std::max(top_padding, padding.top);
                    }
                    if (found_end) break;

                    if (text_start < text.size()) break;
                    text_start = 0;
                }

                if (style.max_lines == current_line && cols_occupied > max_x) {
                    if (style.overflow == TextOverflow::ellipsis) {
                        auto st = as.strings[std::min(chunk_start, as.strings.size() - 1)].second.to_style(style);
                        start_x = std::max({x, max_x, dsize_t{3}}) - 3;
                        for (auto i = 0ul; i < 3 && start_x < max_x; ++i) {
                            if constexpr (ShouldDraw) {
                                draw_pixel(start_x++, y, ".", st);
                            }
                        }
                    }
                }

                x = std::min(max_x, x);
                return { needs_underline, consumed };
            };

            auto needs_underline{false};

            auto consumed_text = std::size_t{};
            auto chunk_end = std::string_view::npos;
            auto text_end = std::string_view::npos;
            if (style.break_whitespace) {
                text_end = width_info.word_boundary;
            }

            if (!style.word_wrap) {
                auto [underline, consumed] = render(
                    chunk_start,
                    text_start,
                    {},
                    width_info.cols_occupied,
                    chunk_end,
                    text_end
                );
                needs_underline = underline;
                consumed_text = consumed;
            } else {
                if (current_line < style.max_lines || style.overflow == TextOverflow::none) {
                    auto [underline, consumed] = render(
                        chunk_start,
                        text_start,
                        {},
                        width_info.cols_occupied,
                        chunk_end,
                        text_end
                    );
                    needs_underline = underline;
                    consumed_text = consumed;
                } else {
                    auto cols_occupied = width_info.cols_occupied;

                    width_info = measure_single_line_text_width_helper(
                        as,
                        chunk_start,
                        text_start,
                        container,
                        style,
                        cols_occupied
                    );

                    if (style.break_whitespace) {
                        text_end = width_info.word_boundary;
                    }

                    auto overflow_span = Span();
                    if (style.overflow == TextOverflow::start_ellipsis) {
                        overflow_span = width_info.start_overflow;
                        overflow_span = Span(
                            overflow_span.start(),
                            overflow_span.end() + 6
                        );
                    } else if (style.overflow == TextOverflow::middle_ellipsis) {
                        overflow_span = width_info.middle_overflow;
                    }

                    auto [underline, consumed] = render(
                        chunk_start,
                        text_start,
                        overflow_span,
                        cols_occupied,
                        chunk_end,
                        text_end
                    );
                    needs_underline = underline;
                    consumed_text = consumed;
                }
            }

            if (current_line == style.max_lines) {
                bottom_padding = std::max(style.padding.bottom, bottom_padding) - style.padding.bottom;
            }

            return {
                chunk_start,
                text_start,
                bottom_padding + top_padding + static_cast<unsigned>(needs_underline),
                consumed_text
            };
        }
    private:
        size_type m_rows{};
        size_type m_cols{};
        std::vector<Cell> m_cells;
        dsize_t m_max_rows_written{};
    };

} // namespace dark::term

template <>
struct std::hash<dark::term::Point> {
    constexpr auto operator()(dark::term::Point const& p) const noexcept -> std::size_t {
        auto h0 = std::hash<unsigned>{}(p.x);
        auto h1 = std::hash<unsigned>{}(p.y);
        return h0 ^ (h1 << 1);
    }
};

template <>
struct std::formatter<dark::term::Point> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::term::Point const& p, auto& ctx) const {
        return std::format_to(ctx.out(), "Point(x: {}, y: {})", p.x, p.y);
    }
};

#endif // AMT_DARK_DIAGNOSTICS_CORE_TERM_CANVAS_HPP
