#ifndef DARK_DIAGNOSTICS_CORE_UTF8_HPP
#define DARK_DIAGNOSTICS_CORE_UTF8_HPP

#include <array>

namespace dark::core::utf8 {
    static constexpr std::array<std::uint8_t, 16> lookup {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 2, 2, 3, 4
    };

    constexpr auto get_length(char c) -> std::uint8_t {
        auto const byte = static_cast<std::uint8_t>(c);
        return lookup[byte >> 4];
    }
} // namespace dark::core::utf8

#endif // DARK_DIAGNOSTICS_CORE_UTF8_HPP
