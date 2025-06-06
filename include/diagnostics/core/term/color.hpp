#ifndef AMT_DARK_DIAGNOSTICS_CORE_TERM_COLOR_HPP
#define AMT_DARK_DIAGNOSTICS_CORE_TERM_COLOR_HPP

#include <cstdint>
#include <format>

namespace dark {
    enum class Color: std::uint8_t {
        Black = 0,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
        Current,
    };
} // namespace dark

template <>
struct std::formatter<dark::Color> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::Color const& c, auto& ctx) const {
        auto name = "";
        switch (c) {
        case dark::Color::Current: name = "Current"; break;
        case dark::Color::Black: name = "Black"; break;
        case dark::Color::Red: name = "Red"; break;
        case dark::Color::Green: name = "Green"; break;
        case dark::Color::Yellow: name = "Yellow"; break;
        case dark::Color::Blue: name = "Blue"; break;
        case dark::Color::Magenta: name = "Magenta"; break;
        case dark::Color::Cyan: name = "Cyan"; break;
        case dark::Color::White: name = "White"; break;
        }
        return std::format_to(ctx.out(), "{}", name);
    }
};

#endif // AMT_DARK_DIAGNOSTICS_CORE_TERM_COLOR_HPP
