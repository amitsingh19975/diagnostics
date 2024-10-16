#ifndef DARK_DIAGNOSTICS_CORE_COW_STRING_HPP
#define DARK_DIAGNOSTICS_CORE_COW_STRING_HPP

#include "string_utils.hpp"
#include "utf8.hpp"
#include <stddef.h>
#include <string>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <variant>

namespace dark::core {
    namespace detail {
        struct owned_tag {};
        struct borrowed_tag {};

        template <typename T>
        concept is_convertible_to_cow_string = std::disjunction_v<std::is_convertible<T, std::string>, std::is_convertible<T, std::string_view>>;

        template <typename T>
        struct is_owned_tag: std::is_same<T, owned_tag> {};

        template <typename T>
        struct is_borrowed_tag: std::is_same<T, borrowed_tag> {};

        template <typename T>
        concept is_cow_string_tag = is_owned_tag<T>::value || is_borrowed_tag<T>::value;

        template <typename T>
        struct is_array_of_chars: std::false_type{};

        template <std::size_t N>
        struct is_array_of_chars<char(&)[N]>: std::true_type{};

        template <std::size_t N>
        struct is_array_of_chars<char[N]>: std::true_type{};

        template <std::size_t N>
        struct is_array_of_chars<char const(&)[N]>: std::true_type{};

        template <std::size_t N>
        struct is_array_of_chars<char const[N]>: std::true_type{};

        template <typename T>
        struct is_std_string: std::is_same<std::decay_t<std::remove_cvref_t<T>>, std::string> {};
    } // namespace detail

    static constexpr auto owned_t = detail::owned_tag{};
    static constexpr auto borrowed_t = detail::borrowed_tag{};

    class CowString {
    public:
        using size_type = std::size_t;
        static constexpr size_type npos = std::string::npos;

        constexpr CowString() noexcept = default;
        constexpr CowString(CowString const&) = default;
        CowString& operator=(CowString const&) = default;
        constexpr CowString(CowString&&) noexcept = default;
        CowString& operator=(CowString&&) noexcept = default;
        ~CowString() = default;

        constexpr CowString(std::string_view s) noexcept
            : m_data(s)
        {}

        constexpr CowString(std::string_view s, detail::is_cow_string_tag auto tag) {
            if constexpr (detail::is_owned_tag<decltype(tag)>::value) {
                m_data = std::string(s);
            } else {
                m_data = s;
            }
        }

        template<detail::is_convertible_to_cow_string T>
        constexpr CowString(T&& str) noexcept {
            if constexpr (detail::is_array_of_chars<T>::value) {
                m_data = std::string_view(str);
            } else if constexpr (detail::is_std_string<T>::value && !std::is_lvalue_reference_v<T>) {
                m_data = std::move(str);
            } else {
                m_data = std::string(str);
            }
        }

        friend auto operator<<(std::ostream& os, CowString const& s) -> std::ostream& {
            return os << s.to_borrowed();
        }

        constexpr auto is_owned() const noexcept -> bool {
            return m_data.index() == 1;
        }

        constexpr auto is_borrowed() const noexcept -> bool {
            return m_data.index() == 0;
        }

        constexpr auto size() const noexcept -> size_type {
            return std::visit([](auto const& s) { return s.size(); }, m_data);
        }

        constexpr auto empty() const noexcept -> bool {
            return std::visit([](auto const& s) { return s.empty(); }, m_data);
        }

        auto to_owned() const -> std::string {
            return std::string(to_borrowed());
        }

        constexpr auto to_borrowed() const noexcept -> std::string_view {
            return std::visit([](auto const& s) { return std::string_view(s); }, m_data);
        }

        constexpr auto operator==(CowString const& other) const noexcept {
            return m_data == other.m_data;
        }

        constexpr auto operator!=(CowString const& other) const noexcept {
            return !(*this == other);
        }

        constexpr auto operator==(std::string_view other) const noexcept {
            return to_borrowed() == other;
        }

        constexpr friend auto operator==(std::string_view lhs, CowString const& rhs) noexcept {
            return (rhs == lhs);
        }

        constexpr auto operator==(char const* other) const noexcept {
            return to_borrowed() == std::string_view(other);
        }

        constexpr friend auto operator==(char const* lhs, CowString const& rhs) noexcept {
            return (rhs == lhs);
        }

        constexpr auto operator!=(std::string_view other) const noexcept {
            return !(*this == other);
        }

        constexpr friend auto operator!=(std::string_view lhs, CowString const& rhs) noexcept {
            return (rhs != lhs);
        }

        constexpr operator bool() const noexcept {
            return !to_borrowed().empty();
        }

        constexpr auto trim() const noexcept -> std::string_view {
            return utils::trim(to_borrowed());
        }

        auto substr(size_type pos, size_type n = std::string::npos) const -> CowString {
            if (is_owned()) {
                auto const& temp = std::get<1>(m_data);
                return temp.substr(pos, n);
            } else {
                return std::get<0>(m_data).substr(pos, n);
            }
        }
        
        constexpr auto utf8_char_count() const noexcept {
            auto s = to_borrowed();
            auto i = 0zu;
            auto count = 0zu;
            while (i < s.size()) {
                auto len = core::utf8::get_length(s[i]);
                ++count;
                i += len;
            }

            return count;
        }

    private:
        std::variant<std::string_view, std::string> m_data{ std::string_view{} };
    };

} // namespace dark::core

#endif // DARK_DIAGNOSTICS_CORE_COW_STRING_HPP
