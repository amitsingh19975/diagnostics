#ifndef AMT_DARK_DIAGNOSTICS_CORE_WRITER_HPP
#define AMT_DARK_DIAGNOSTICS_CORE_WRITER_HPP

#include "color.hpp"
#include "config.hpp"
#include "style.hpp"
#include <cstdio>
#include <print>
#include <utility>

namespace dark {
    template <typename C>
    struct Writer;

    template <>
    struct Writer<FILE*> {
        constexpr Writer(FILE* handle) noexcept
            : m_handle(handle)
        {}

        auto is_displayed() const noexcept -> bool {
            return core::term::is_displayed(m_handle);
        }

        constexpr auto get_handle() const noexcept -> FILE* {
            return m_handle;
        }

        auto write(std::string_view str) -> void {
            std::print(m_handle, "{}", str);
        }

        template <typename... Args>
        auto write(std::format_string<Args...> fmt, Args&&... args) -> void {
            std::print(m_handle, fmt, std::forward<Args>(args)...);
        }

        auto flush() noexcept -> void {
            std::fflush(m_handle);
        }

        auto columns() noexcept -> std::size_t {
            return core::term::get_columns(m_handle);
        }

    private:
        FILE* m_handle;
    };

    namespace detail {
        template <typename T>
        concept WriterHasHandle = requires (Writer<T> const& w) {
            { w.get_handle() } -> std::same_as<FILE*>;
        };
    }
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_CORE_WRITER_HPP

