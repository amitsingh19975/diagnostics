#ifndef AMT_DARK_DIAGNOSTICS_CORE_TERM_COLOR_HPP
#define AMT_DARK_DIAGNOSTICS_CORE_TERM_COLOR_HPP

#include <bit>
#include <cstdint>
#include <format>
#include <limits>

namespace dark {
    struct Color {
        using value_type = std::uint8_t;
        struct reserved_t{};

        // [rrggbb][8-bit color for backward compatibility]
        static constexpr auto invalid = std::numeric_limits<value_type>::max();
        static constexpr auto rgb = value_type{100};
        value_type r{};
        value_type g{};
        value_type b{};
        value_type reserved{invalid};

        constexpr Color() noexcept = default;
        constexpr Color(Color const&) noexcept = default;
        constexpr Color(Color &&) noexcept = default;
        constexpr Color& operator=(Color const&) noexcept = default;
        constexpr Color& operator=(Color &&) noexcept = default;
        constexpr ~Color() noexcept = default;

        constexpr Color(value_type r) noexcept
            : r(r)
            , g(r)
            , b(r)
            , reserved(rgb)
        {}

        constexpr Color(value_type v, reserved_t) noexcept
            : reserved(v)
        {}

        constexpr Color(value_type r, value_type g, value_type b) noexcept
            : r(r)
            , g(g)
            , b(b)
            , reserved(rgb)
        {}

        constexpr auto operator==(Color const&) const noexcept -> bool = default;

        constexpr auto to_int() const noexcept -> std::uint32_t {
            return std::bit_cast<std::uint32_t>(*this);
        }

        constexpr auto is_invalid() const noexcept -> bool {
            return reserved == invalid;
        }

        constexpr auto is_rgb() const noexcept -> bool {
            return reserved == rgb;
        }

        static const Color Black;
        static const Color Red;
        static const Color Green;
        static const Color Yellow;
        static const Color Blue;
        static const Color Magenta;
        static const Color Cyan;
        static const Color White;
        static const Color BrightBlack;
        static const Color BrightRed;
        static const Color BrightGreen;
        static const Color BrightYellow;
        static const Color BrightBlue;
        static const Color BrightMagenta;
        static const Color BrightCyan;
        static const Color BrightWhite;
        static const Color Default;
    };

    inline constexpr auto Color::Black              = Color{ 0, Color::reserved_t{} };
    inline constexpr auto Color::Red                = Color{ 1, Color::reserved_t{} };
    inline constexpr auto Color::Green              = Color{ 2, Color::reserved_t{} };
    inline constexpr auto Color::Yellow             = Color{ 3, Color::reserved_t{} };
    inline constexpr auto Color::Blue               = Color{ 4, Color::reserved_t{} };
    inline constexpr auto Color::Magenta            = Color{ 5, Color::reserved_t{} };
    inline constexpr auto Color::Cyan               = Color{ 6, Color::reserved_t{} };
    inline constexpr auto Color::White              = Color{ 7, Color::reserved_t{} };
    inline constexpr auto Color::BrightBlack        = Color{ 8, Color::reserved_t{} };
    inline constexpr auto Color::BrightRed          = Color{ 9, Color::reserved_t{} };
    inline constexpr auto Color::BrightGreen        = Color{10, Color::reserved_t{} };
    inline constexpr auto Color::BrightYellow       = Color{11, Color::reserved_t{} };
    inline constexpr auto Color::BrightBlue         = Color{12, Color::reserved_t{} };
    inline constexpr auto Color::BrightMagenta      = Color{13, Color::reserved_t{} };
    inline constexpr auto Color::BrightCyan         = Color{14, Color::reserved_t{} };
    inline constexpr auto Color::BrightWhite        = Color{15, Color::reserved_t{} };
    inline constexpr auto Color::Default            = Color{16, Color::reserved_t{} };
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
        switch (c.to_int()) {
        case dark::Color::Default.to_int(): name = "Default"; break;
        case dark::Color::Black.to_int(): name = "Black"; break;
        case dark::Color::Red.to_int(): name = "Red"; break;
        case dark::Color::Green.to_int(): name = "Green"; break;
        case dark::Color::Yellow.to_int(): name = "Yellow"; break;
        case dark::Color::Blue.to_int(): name = "Blue"; break;
        case dark::Color::Magenta.to_int(): name = "Magenta"; break;
        case dark::Color::Cyan.to_int(): name = "Cyan"; break;
        case dark::Color::White.to_int(): name = "White"; break;
        default: {
            return std::format_to(ctx.out(), "RGB({}, {}, {})", c.r, c.g, c.b);
        }
        }
        return std::format_to(ctx.out(), "{}", name);
    }
};

#endif // AMT_DARK_DIAGNOSTICS_CORE_TERM_COLOR_HPP
