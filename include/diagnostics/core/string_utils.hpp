#ifndef DARK_DIAGNOSTICS_CORE_STRING_UTILS_HPP
#define DARK_DIAGNOSTICS_CORE_STRING_UTILS_HPP

#include <string>

namespace dark::core::utils {

    inline static auto ltrim(std::string& str, std::string_view chars = " \t\n\r\f\v") -> std::string& {
        str.erase(0, str.find_first_not_of(chars));
        return str;
    }

    inline static auto rtrim(std::string& str, std::string_view chars = " \t\n\r\f\v") -> std::string& {
        str.erase(str.find_last_not_of(chars) + 1);
        return str;
    }

    inline static auto trim(std::string& str, std::string_view chars = " \t\n\r\f\v") -> std::string& {
        return ltrim(rtrim(str, chars), chars);
    }

    static constexpr auto ltrim(std::string_view str, std::string_view chars = " \t\n\r\f\v") noexcept -> std::string_view {
        auto const it = str.find_first_not_of(chars);
        if (it == std::string_view::npos) return str;
        return str.substr(it);
    }

    static constexpr auto rtrim(std::string_view str, std::string_view chars = " \t\n\r\f\v") noexcept -> std::string_view {
        auto const it = str.find_last_not_of(chars);
        if (it == std::string_view::npos) return str;
        return str.substr(0, it + 1);
    }

    static constexpr auto trim(std::string_view str, std::string_view chars = " \t\n\r\f\v") noexcept -> std::string_view {
        return ltrim(rtrim(str, chars), chars);
    }

} // namespace dark::core::utils

#endif // DARK_DIAGNOSTICS_CORE_STRING_UTILS_HPP
