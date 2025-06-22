#ifndef AMT_DARK_DIAGNOSTIC_CORE_FORMAT_HPP
#define AMT_DARK_DIAGNOSTIC_CORE_FORMAT_HPP

#include "cow_string.hpp"
#include "small_vec.hpp"
#include "format_any.hpp"
#include <format>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace dark {
    namespace core {
        template <typename... Args>
        using format_string = std::format_string<
            std::conditional_t<!std::same_as<std::remove_cvref_t<Args>, FormatterAnyArg>, FormatterAnyArg, Args>...
        >;
        /*
         * @brief Reasons for creating custom format class that holds args.
         *        1. `std::make_format_args` cannot be stored during runtime and will cause
         *        segmentation fault. Therefore, we need a holder that holds any args during runtime.
         *        2. `FormatterAnyArg` can hold any value that implements methods `std::to_string`,
         *        `::to_string`, `std::ostream` or `std::formatter`.
         * @code
         *  struct foo { int a; };
         *  std::string to_string(foo f) { return std::to_string(f.a); }
         *  auto f = BasicFormatter("{}", foo(12));
         *  std::println("{}", f)
         * @endcode
         *
         * @code
         *  struct foo { int a; };
         *  std::ostream& operator<<(std::ostream& os, foo const& f) { return os << f.a; }
         *  auto f = BasicFormatter("{}", foo(12));
         *  std::println("{}", f)
         * @endcode
         *
         * @code
         *  struct foo { int a; };
         *  struct std::formatter<foo> { ... };
         *  auto f = BasicFormatter("{}", foo(12));
         *  std::println("{}", f)
         * @endcode
         */
        struct BasicFormatter {
            constexpr BasicFormatter() noexcept = default;
            BasicFormatter(BasicFormatter const&) = delete;
            BasicFormatter(BasicFormatter&& other) noexcept
                : m_format(std::move(other.m_format))
                , m_args(std::move(other.m_args))
                , m_apply(std::exchange(other.m_apply, nullptr))
            {}
            BasicFormatter& operator=(BasicFormatter const&) = delete;
            BasicFormatter& operator=(BasicFormatter&& other) noexcept {
                if (this == &other) return *this;
                auto tmp = BasicFormatter(std::move(other));
                swap(*this, tmp);
                return *this;
            }

            ~BasicFormatter() = default;

            template <IsFormattable... Args>
            BasicFormatter(format_string<Args...> fmt, Args&&... args) 
                : m_format(std::move(fmt.get()))
            {
                m_args.reserve(sizeof...(args));
                (m_args.emplace_back(std::move(args)),...);
                m_apply = [](std::string_view fmt, std::span<FormatterAnyArg const> args) -> std::string {
                    auto helper = []<std::size_t... Is>(
                        std::string_view fmt,
                        std::span<FormatterAnyArg const> args,
                        std::index_sequence<Is...>
                    ) {
                        return std::vformat(fmt, std::make_format_args(args[Is]...));
                    };
                    return helper(fmt, args, std::make_index_sequence<sizeof...(Args)>{});
                };
            }

            BasicFormatter(CowString format)
                : m_format(std::move(format))
            {}

            constexpr auto empty() const noexcept -> bool {
                return m_format.empty();
            }

            constexpr operator bool() const noexcept {
                return (m_apply != nullptr) && !empty();
            }

            auto format() const -> CowString {
                if (empty()) return "";
                if (!m_apply) return CowString(m_format.to_borrowed());
                return CowString(m_apply(m_format, std::span(m_args)));
            }

            constexpr friend auto swap(BasicFormatter& lhs, BasicFormatter& rhs) noexcept -> void {
                using std::swap;
                swap(lhs.m_format, rhs.m_format);
                swap(lhs.m_args, rhs.m_args);
                swap(lhs.m_apply, rhs.m_apply);
            }

        private:
            CowString m_format;
            SmallVec<FormatterAnyArg, 4> m_args;
            std::function<std::string(std::string_view, std::span<FormatterAnyArg const>)> m_apply{nullptr};
        };

    } // namespace core
} // namespace dark

template <>
struct std::formatter<dark::core::BasicFormatter> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::core::BasicFormatter const& f, auto& ctx) const {
        return std::format_to(ctx.out(), "{}", f.format().to_borrowed());
    }
};
#endif // AMT_DARK_DIAGNOSTIC_CORE_FORMAT_HPP
