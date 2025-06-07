#ifndef AMT_DARK_DIAGNOSTICS_TERM_STYLE_HPP
#define AMT_DARK_DIAGNOSTICS_TERM_STYLE_HPP

#include "color.hpp"
#include <limits>

namespace dark::term {
    struct Style {
        Color text_color{ Color::Default };
        Color bg_color{ Color::Default };
        bool bold{false};
        bool dim{false};
        bool strike{false};
        bool italic{false};
        unsigned groupId{};
        int zIndex{};

        static constexpr auto invalid_group_id = 0;
        static constexpr auto line_group_id = 1;

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

    enum class TextAlign {
        Left,
        Center,
        Right
    };

    struct TextStyle {
        Color text_color{ Color::Default };
        Color bg_color{ Color::Default };
        bool bold{false};
        bool dim{false};
        bool strike{false};
        bool italic{false};
        unsigned groupId{};
        int zIndex{};
        bool word_wrap{true};
        bool break_whitespace{};
        unsigned max_lines{ std::numeric_limits<unsigned>::max() };
        unsigned max_width{ std::numeric_limits<unsigned>::max() };
        PaddingValues padding{};
        TextAlign align{ TextAlign::Left };
        TextOverflow overflow{ TextOverflow::none };
        bool trim_prefix{false};

        constexpr auto to_style() const noexcept -> Style {
            return {
                .text_color = text_color,
                .bg_color = bg_color,
                .bold = bold,
                .dim = dim,
                .strike = strike,
                .italic = italic,
                .groupId = groupId,
                .zIndex = zIndex,
            };
        }

        static constexpr auto default_header_style() noexcept -> TextStyle {
            return {
                .bold = true,
                .word_wrap = false,
                .overflow = TextOverflow::ellipsis,
                .trim_prefix = true
            };
        }
    };
} // namespace dark::term

template <>
struct std::formatter<dark::term::PaddingValues> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::term::PaddingValues const& p, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "PaddingValues(top={}, right={}, bottom={}, left={})", p.top, p.right, p.bottom, p.left);
        return out;
    }
};
#endif // AMT_DARK_DIAGNOSTICS_TERM_STYLE_HPP
