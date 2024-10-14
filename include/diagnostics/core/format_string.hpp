#ifndef DARK_DIAGNOSTICS_CORE_FORMAT_STRING_HPP
#define DARK_DIAGNOSTICS_CORE_FORMAT_STRING_HPP

#include "static_string.hpp"
#include "string_utils.hpp"
#include "cow_string.hpp"
#include <array>
#include <cstddef>
#include <numeric>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace dark::core {

    namespace internal {
        static constexpr std::string_view type_mapping[] = {
            "c",
            "s",
            "u8",
            "u16",
            "u32",
            "u64",
            "i8",
            "i16",
            "i32",
            "i64",
            "f32",
            "f64",
            "any"
        };
        static constexpr auto total_types = sizeof(type_mapping) / sizeof(type_mapping[0]);

        template <std::size_t MaxArgs>
        constexpr auto parse_format_types(std::string_view str) noexcept
            -> std::pair<std::array<std::string_view, MaxArgs>, std::size_t>
        {
            std::array<std::string_view, MaxArgs> res{};
            if (str.size() < 2) return std::make_pair(res, 0);

            auto size = 0zu;
            for (auto i = 0zu; i < str.size() - 1; ++i) {
                auto const c = str[i];
                switch (c) {
                    case '{': {
                        if (str[i + 1] == '{') {
                            i += 1;
                            continue;
                        }
                        ++i;
                        while (i < str.size() && str[i] == ' ') ++i;
                        if (i >= str.size()) std::make_pair(res, size);

                        auto start = i;

                        while (i < str.size() && (str[i] != '}' && str[i] != ':')) ++i;
                        if (i >= str.size()) std::make_pair(res, size);

                        auto s_size = i - start;

                        res[size++] = utils::trim(str.substr(start, s_size));

                        while (i < str.size() && str[i] != '}') ++i;
                        if (i >= str.size()) std::make_pair(res, size);

                    } break;
                }
            }
            return std::make_pair(res, size);
        }

        constexpr auto type_to_index_mapper_helper(std::string_view t) -> std::size_t {
            if (t.empty()) return total_types - 1;

            for (auto i = 0ul; i < total_types - 1; ++i) {
                if (type_mapping[i] == t) return i;
            }
            return total_types;
        }

        template <std::size_t N, std::size_t M>
        constexpr auto type_to_index_mapper(std::array<std::string_view, M> arr) {
            std::array<std::size_t, N> res{};
            for (auto i = 0ul; i < N; ++i) {
               res[i] = type_to_index_mapper_helper(arr[i]);
            }
            return res;
        }

        template <std::size_t N>
        constexpr auto have_valid_type_ids(std::array<std::size_t, N> const& arr) noexcept -> bool {
            for (auto const& e : arr) {
                if (e == total_types) return false;
            }
            return true;
        }

        template <std::size_t TypeId>
        constexpr auto type_id_to_type() noexcept {
            constexpr auto s = type_mapping[TypeId];
            if constexpr (s == "s") return std::type_identity<CowString>{};
            else if constexpr (s == "c") return std::type_identity<char>{};
            else if constexpr (s == "u8") return std::type_identity<std::uint8_t>{};
            else if constexpr (s == "u16") return std::type_identity<std::uint16_t>{};
            else if constexpr (s == "u32") return std::type_identity<std::uint32_t>{};
            else if constexpr (s == "u64") return std::type_identity<std::uint64_t>{};
            else if constexpr (s == "i8") return std::type_identity<std::int8_t>{};
            else if constexpr (s == "i16") return std::type_identity<std::int16_t>{};
            else if constexpr (s == "i32") return std::type_identity<std::int32_t>{};
            else if constexpr (s == "i64") return std::type_identity<std::int64_t>{};
            else if constexpr (s == "f32") return std::type_identity<float>{};
            else if constexpr (s == "f64") return std::type_identity<double>{};
            else return std::type_identity<void>{};
        }

        template <StaticString Str, std::size_t MaxArgs = 21>
        constexpr auto extract_format_types() noexcept {
            constexpr auto res = parse_format_types<MaxArgs>(Str);
            constexpr auto ts = type_to_index_mapper<res.second>(res.first);

            static_assert(have_valid_type_ids(ts), "Invalid type found. Supported types 'c', 's', 'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64', or no type for any type.");

            constexpr auto helper = [ts]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::tuple<
                    decltype(type_id_to_type<ts[Is]>())...
                >{};
            };

            return helper(std::make_index_sequence<ts.size()>{});
        }

        constexpr auto remove_types_from_format_string(CowString& str) {
            auto s = str.to_borrowed();
            auto const [ts, size] = parse_format_types<21zu>(s);

            auto total_any_types_provided = std::accumulate(ts.begin(), ts.begin() + size, 0zu, [](auto a, std::string_view e) {
                return a + static_cast<std::size_t>(e.empty());
            });

            if (size == 0 || total_any_types_provided == size) return;

            std::string res;
            res.reserve(str.size());

            auto last = s.begin();
            for (auto i = 0zu; i < size; ++i) {
                auto const el = ts[i];
                auto first = static_cast<std::size_t>(el.begin() - last);
                res += s.substr(static_cast<std::size_t>(last - s.begin()), first);
                last = el.end();
            }

            if (last != s.end()) {
                res += s.substr(static_cast<std::size_t>(last - s.begin()));
            }

            res.shrink_to_fit();
            str = std::move(res);
        }

        template <StaticString Str, std::size_t MaxArgs = 21>
        using extract_format_types_t = decltype(extract_format_types<Str, MaxArgs>());
    } // namespace internal

} // namespace dark::core

#endif // DARK_DIAGNOSTICS_CORE_FORMAT_STRING_HPP
