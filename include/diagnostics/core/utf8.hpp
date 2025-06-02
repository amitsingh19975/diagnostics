#ifndef AMT_DARK_DIAGNOSTIC_CORE_UTF8_HPP
#define AMT_DARK_DIAGNOSTIC_CORE_UTF8_HPP

#include <array>
#include "diagnostics/core/config.hpp"
#include "small_vec.hpp"

namespace dark::core::utf8 {
    static constexpr std::array<std::uint8_t, 16> lookup {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 2, 2, 3, 4
    };

    constexpr auto get_length(char c) -> std::uint8_t {
        auto const byte = static_cast<std::uint8_t>(c);
        return lookup[byte >> 4];
    }

    struct PackedUTF8 {
    private:
        struct Wrapper {
            char data[4] = {};
            std::uint8_t len{};
        };
    public:
        using size_type = std::size_t;

        PackedUTF8() noexcept = default;
        PackedUTF8(PackedUTF8 const&) = default;
        PackedUTF8(PackedUTF8 &&) noexcept = default;
        PackedUTF8& operator=(PackedUTF8 const&) = default;
        PackedUTF8& operator=(PackedUTF8 &&) noexcept = default;
        ~PackedUTF8() = default;

        // NOTE: Assumes valid utf-8 string
        PackedUTF8(std::string_view str) {
            m_data.reserve(str.size());
            for (auto i = 0ul; i < str.size();) {
                auto len = get_length(str[i]);
                DARK_ASSUME(len <= 4);

                auto s = i;
                i += len;
                Wrapper tmp = { .data = {}, .len = len };
                for (auto j = 0ul; j < len; ++j) {
                    tmp.data[j] = str[s + j];
                }
                m_data.push_back(tmp);
            }
        }

        auto operator[](size_type k) noexcept -> std::string_view {
            auto& tmp = m_data[k];
            return { tmp.data, tmp.len };
        }

        auto operator[](size_type k) const noexcept -> std::string_view {
            auto const& tmp = m_data[k];
            return { tmp.data, tmp.len };
        }

        constexpr auto size() const noexcept -> size_type { return m_data.size(); }

        auto to_string() const -> std::string {
            std::string res;
            res.reserve(size() * 4);
            for (auto i = 0ul; i < size(); ++i) {
                res += this->operator[](i);
            }
            return res;
        }

        auto find(std::string_view pattern) const noexcept -> size_type {
            for (auto i = 0ul; i < size(); ++i) {
                if (this->operator[](i) == pattern) return i;
            }
            return std::string::npos;
        }
    private:
        SmallVec<Wrapper, 64> m_data;
    };
} // namespace dark::core::utf8

#endif // AMT_DARK_DIAGNOSTIC_CORE_UTF8_HPP
