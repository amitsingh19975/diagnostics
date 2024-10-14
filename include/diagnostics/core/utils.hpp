#ifndef DARK_DIAGNOSTICS_CORE_UTILS_HPP
#define DARK_DIAGNOSTICS_CORE_UTILS_HPP

namespace dark::core::utils {

    template <typename... Ts>
    struct overloaded: Ts... { using Ts::operator()...; };

    template <typename... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

} // namespace dark::core::utils

#endif // DARK_DIAGNOSTICS_CORE_UTILS_HPP
