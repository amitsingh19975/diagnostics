#ifndef DARK_DIAGNOSTIC_CORE_STATIC_STRING_HPP
#define DARK_DIAGNOSTIC_CORE_STATIC_STRING_HPP

#include <ostream>
#include <string_view>

namespace dark {

    template <typename CharT, std::size_t N>
    struct BasicStaticString {
        CharT m_data[N];

        constexpr BasicStaticString(CharT const (&str)[N + 1]) {
            for (std::size_t i = 0; i < N; ++i) {
                m_data[i] = str[i];
            }
        }

        constexpr BasicStaticString(std::string_view str) {
            for (std::size_t i = 0; i < std::min(N, str.size()); ++i) {
                m_data[i] = str[i];
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const BasicStaticString& str) {
            os << str.m_data;
            return os;
        }

        constexpr auto operator==(std::string_view other) const -> bool {
            if (other.size() != N) return false;

            for (std::size_t i = 0; i < N; ++i) {
                if (m_data[i] != other[i]) {
                    return false;
                }
            }
            return true;
        }

        constexpr operator std::string_view() const {
            return std::string_view(m_data, N);
        }

        constexpr auto size() const -> std::size_t {
            return N;
        }

        constexpr auto data() const -> const CharT* {
            return m_data;
        }

        constexpr auto operator[](std::size_t index) const -> CharT {
            return m_data[index];
        }
    };

    template <typename CharT, std::size_t N>
    BasicStaticString(const CharT (&)[N]) -> BasicStaticString<CharT, N - 1>;

    template <std::size_t N>
    struct StaticString : BasicStaticString<char, N> {
        using BasicStaticString<char, N>::BasicStaticString;

        template<typename... Ts>
        constexpr StaticString(Ts&&... ts) : BasicStaticString<char, N>{std::forward<Ts>(ts)...} {}
    };

    template <std::size_t N>
    StaticString(const char (&)[N]) -> StaticString<N - 1>;

} // namespace dark

#endif // DARK_DIAGNOSTIC_CORE_STATIC_STRING_HPP
