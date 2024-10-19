#ifndef DARK_DIAGNOSTICS_CORE_FORMATTER_HPP
#define DARK_DIAGNOSTICS_CORE_FORMATTER_HPP

#include "../core/cow_string.hpp"
#include "../core/format_string.hpp"
#include "../core/small_vector.hpp"
#include "../core/static_string.hpp"
#include "../core/stream.hpp"
#include <iostream>
#include <sstream>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <format>
#include <variant>
#include <concepts>

namespace dark::core {
    [[maybe_unused]] static inline auto to_formatter_arg(CowString const& s) -> CowString {
        return s;
    }

    namespace detail {
        template <typename T>
        concept is_basic_valid_formatter_arg = std::disjunction_v<
            std::is_same<T, CowString>,
            std::is_constructible<CowString, T>,
            std::is_same<T, char>,
            std::is_same<T, uint8_t>,
            std::is_same<T, uint16_t>,
            std::is_same<T, uint32_t>,
            std::is_same<T, uint64_t>,
            std::is_same<T, int8_t>,
            std::is_same<T, int16_t>,
            std::is_same<T, int32_t>,
            std::is_same<T, int64_t>,
            std::is_same<T, float>,
            std::is_same<T, double>,
            std::is_same<T, long double>
        >;

        template <typename T>
        concept can_be_formatted_using_fn = requires(T const& t) {
            { to_formatter_arg(t) } -> is_basic_valid_formatter_arg;
        };

        template <typename T>
        concept can_be_formatted = can_be_formatted_using_fn<T> || ::dark::detail::is_ostreamable<T>;

        template <typename T>
        struct is_valid_formatter_arg_helper: std::bool_constant<is_basic_valid_formatter_arg<T>> {};

        template <can_be_formatted T>
        struct is_valid_formatter_arg_helper<T>: std::true_type {};

        template <typename T>
        concept is_valid_formatter_arg = is_valid_formatter_arg_helper<std::decay_t<std::remove_cvref_t<T>>>::value;

        constexpr decltype(auto) fix_format_arg(is_valid_formatter_arg auto&& arg) {
            using type = std::decay_t<std::remove_cvref_t<decltype(arg)>>;
            if constexpr (std::same_as<type, CowString>) return arg;
            else if constexpr (std::same_as<type, char>) return arg;
            else if constexpr (std::same_as<type, uint8_t>) return arg;
            else if constexpr (std::same_as<type, uint16_t>) return arg;
            else if constexpr (std::same_as<type, uint32_t>) return arg;
            else if constexpr (std::same_as<type, uint64_t>) return arg;
            else if constexpr (std::same_as<type, int8_t>) return arg;
            else if constexpr (std::same_as<type, int16_t>) return arg;
            else if constexpr (std::same_as<type, int32_t>) return arg;
            else if constexpr (std::same_as<type, int64_t>) return arg;
            else if constexpr (std::same_as<type, float>) return arg;
            else if constexpr (std::same_as<type, double>) return arg;
            else if constexpr (std::same_as<type, long double>) return arg;
            else if constexpr (can_be_formatted_using_fn<type>) return to_formatter_arg(arg);
            else {
                std::ostringstream os;
                os << arg;
                return os.str();
            }
        }

    } // namespace detail

    class Formatter {
    public:
        static constexpr auto max_args = 21zu;

        using basic_argument_type = std::variant<
            CowString,
            char,
            int8_t,
            uint8_t,
            int16_t,
            uint16_t,
            int32_t,
            uint32_t,
            int64_t,
            uint64_t,
            float,
            double,
            long double
        >;
        using arguments_type = SmallVec<basic_argument_type, 3>;
        using size_type = std::size_t;

        Formatter() = default;
        Formatter(Formatter const&) = default;
        Formatter& operator=(Formatter const&) = default;
        constexpr Formatter(Formatter &&) noexcept = default;
        Formatter& operator=(Formatter &&) noexcept = default;
        ~Formatter() = default;


        template <typename... Args>
            requires ((... && detail::is_valid_formatter_arg<Args>) && (sizeof...(Args) < max_args))
        Formatter(CowString s, Args&&... args)
            : m_format_string(std::move(s))
            , m_args{ detail::fix_format_arg(args)... }
        {}

        auto format() const -> std::string {
            internal::remove_types_from_format_string(m_format_string);
            switch (m_args.size()) {
                case  0: return format_helper(std::make_index_sequence< 0>{});
                case  1: return format_helper(std::make_index_sequence< 1>{});
                case  2: return format_helper(std::make_index_sequence< 2>{});
                case  3: return format_helper(std::make_index_sequence< 3>{});
                case  4: return format_helper(std::make_index_sequence< 4>{});
                case  5: return format_helper(std::make_index_sequence< 5>{});
                case  6: return format_helper(std::make_index_sequence< 6>{});
                case  7: return format_helper(std::make_index_sequence< 7>{});
                case  8: return format_helper(std::make_index_sequence< 8>{});
                case  9: return format_helper(std::make_index_sequence< 9>{});
                case 10: return format_helper(std::make_index_sequence<10>{});
                case 11: return format_helper(std::make_index_sequence<11>{});
                case 12: return format_helper(std::make_index_sequence<12>{});
                case 13: return format_helper(std::make_index_sequence<13>{});
                case 14: return format_helper(std::make_index_sequence<14>{});
                case 15: return format_helper(std::make_index_sequence<15>{});
                case 16: return format_helper(std::make_index_sequence<16>{});
                case 17: return format_helper(std::make_index_sequence<17>{});
                case 18: return format_helper(std::make_index_sequence<18>{});
                case 19: return format_helper(std::make_index_sequence<19>{});
                case 20: return format_helper(std::make_index_sequence<20>{});
            }
            std::unreachable();
        }

        constexpr auto number_of_args() const noexcept -> size_type {
            return m_args.size();
        }

        friend auto& operator<<(std::ostream& os, Formatter const& f) {
            return os << f.format();
        }

        constexpr auto empty() const noexcept -> bool {
            return m_format_string.empty();
        }

    private:
        template <std::size_t... Is>
        auto format_helper(std::index_sequence<Is...> /*ids*/) const -> std::string {
            return std::vformat(m_format_string.to_borrowed(), std::make_format_args(m_args[Is]...));
        }
    private:
        mutable CowString m_format_string{};
        arguments_type m_args{};
    };
} // namespace dark::core

namespace std {
    template<>
    struct formatter<::dark::core::Formatter::basic_argument_type, char> {
        template<typename parse_context_t>
        constexpr auto parse(parse_context_t& ctx) -> typename parse_context_t::iterator {
            for(auto it = ctx.begin(); it != ctx.end(); ++it)
            {
                m_format += *it;
                if (*it == '}')
                {
                    return it;
                }
            }
            m_format += '}';
            return ctx.end();
        }

        template<typename fmt_context_t>
        auto format(::dark::core::Formatter::basic_argument_type const& variant, fmt_context_t& ctx) const -> typename fmt_context_t::iterator {
            return std::visit(
                [&ctx, this]<typename T>(T const& v) -> fmt_context_t::iterator {
                    if constexpr (std::is_same_v<std::decay_t<std::remove_cvref_t<T>>, dark::core::CowString>) {
                        std::string_view s = v.to_borrowed();
                        return std::vformat_to(ctx.out(), m_format, std::make_format_args(s));
                    } else {
                        return std::vformat_to(ctx.out(), m_format, std::make_format_args(v));
                    }
                },
                variant
            );
        }
        std::string m_format{"{:"};

    };
}


namespace dark::core::internal {

    template <typename Tuple, typename U, std::size_t I>
    using choose_format_arg_t = std::conditional_t<
        std::is_void_v<typename std::tuple_element_t<I, Tuple>::type>,
        U,
        typename std::tuple_element_t<I, Tuple>::type
    >;

    template <StaticString Str, ::dark::core::detail::is_valid_formatter_arg... Args>
    constexpr auto make_formatter(Args&&...) {
        using type = extract_format_types_t<Str>;
        constexpr auto helper = []<std::size_t... Is>(std::index_sequence<Is...>) {
            return [](choose_format_arg_t<type, Args, Is>&&... args) {
                return Formatter(CowString(std::string_view(Str)), std::forward<choose_format_arg_t<type, Args, Is>>(args)...);
            };
        };
        return helper(std::make_index_sequence<std::tuple_size_v<type>>());
    }

} // namespace dark::core::internal

#define dark_format(f,...) ::dark::core::internal::make_formatter<f>(__VA_ARGS__)(__VA_ARGS__)

#endif // DARK_DIAGNOSTICS_CORE_FORMATTER_HPP
