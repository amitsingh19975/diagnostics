#ifndef AMT_DARK_DIAGNOSTICS_TERM_STYLE_HPP
#define AMT_DARK_DIAGNOSTICS_TERM_STYLE_HPP

#include "terminal.hpp"
#include <limits>

namespace dark::term {
    struct Style {
        Terminal::Color text_color{ Terminal::SAVEDCOLOR };
        Terminal::Color bg_color{ Terminal::SAVEDCOLOR };
        bool bold{false};
        bool dim{false};
        bool strike{false};
        unsigned groupId{};
        int zIndex{};

        static constexpr auto invalid_group_id = 0;
        static constexpr auto line_group_id = 1;

        constexpr auto term_style() const noexcept -> Terminal::Style {
            return {
                .bold = bold,
                .dim = dim,
                .strike = strike
            };
        }

        constexpr auto is_valid_group_id() const noexcept -> bool {
            return groupId == invalid_group_id;
        }
    };

    struct PaddingValues {
        unsigned top{};
        unsigned right{};
        unsigned bottom{};
        unsigned left{};

        static constexpr auto all(unsigned value) noexcept -> PaddingValues {
            return PaddingValues(value, value, value, value);
        }

        constexpr auto vertical() const noexcept -> unsigned { return top + bottom; }
        constexpr auto horizontal() const noexcept -> unsigned { return left + right; }
    };

    enum class TextOverflow {
        none = 0,
        ellipsis,
        middle_ellipsis,
        start_ellipsis
    };

    struct TextStyle {
        Terminal::Color text_color{ Terminal::SAVEDCOLOR };
        Terminal::Color bg_color{ Terminal::SAVEDCOLOR };
        bool bold{false};
        bool dim{false};
        bool strike{false};
        unsigned groupId{};
        int zIndex{};
        bool word_wrap{};
        bool break_whitespace{};
        unsigned max_width{ std::numeric_limits<unsigned>::max() };
        PaddingValues padding{};
        unsigned max_lines{ std::numeric_limits<unsigned>::max() };
        TextOverflow overflow{ TextOverflow::none };

        constexpr auto to_style() const noexcept -> Style {
            return {
                .text_color = text_color,
                .bg_color = bg_color,
                .bold = bold,
                .dim = dim,
                .strike = strike,
                .groupId = groupId,
                .zIndex = zIndex,
            };
        }
    };
} // namespace dark::term

#endif // AMT_DARK_DIAGNOSTICS_TERM_STYLE_HPP
