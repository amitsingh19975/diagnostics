#ifndef AMT_DARK_DIAGNOSTIC_CORE_FORMAT_ANY_HPP
#define AMT_DARK_DIAGNOSTIC_CORE_FORMAT_ANY_HPP

#include <concepts>
#include <cstddef>
#include <format>
#include <functional>
#include <memory_resource>
#include <sstream>
#include <string>
#include <memory.h>
#include <string_view>

namespace dark::core {
    struct FormatterAnyArg;
}

namespace std {
    template<>
    struct formatter<::dark::core::FormatterAnyArg>;
}

namespace dark::core {
    template <typename T>
    concept IsFormattableUsingGlobalOverload = requires (T&& t) {
        { to_string(t) } -> std::same_as<std::string>;
    };

    template <typename T>
    concept IsFormattableUsingStandardToString = requires (T&& t) {
        { std::to_string(t) } -> std::same_as<std::string>;
    };

    template <typename T>
    concept IsFormattableUsingMember = requires (T&& t) {
        { t.to_string() } -> std::same_as<std::string>;
    };

    template <typename T, typename CharT = char>
    concept IsFormattableUsingStandardFormat = requires(T&& t) {
        { std::formatter<T, CharT>{} } -> std::same_as<std::formatter<T, CharT>>;
    };

    template <typename T>
    concept IsFormattableUsingStandardOStream = requires(std::stringstream os, T&& t) {
        { os << t };
    };

    template <typename T>
    concept IsFormattable =
        IsFormattableUsingMember<T> ||
        IsFormattableUsingGlobalOverload<T> ||
        IsFormattableUsingStandardToString<T> ||
        IsFormattableUsingStandardFormat<T> ||
        IsFormattableUsingStandardOStream<T>
        ;

    struct FormatterAnyArg {
        static constexpr auto small_buffer_size = std::max<std::size_t>({
            32,
            sizeof(std::string_view) + alignof(std::string_view),
            sizeof(std::string) + alignof(std::string)
        });
    private:
        struct AnyWrapper {
            union {
                std::byte* ptr{nullptr};
                std::byte data[small_buffer_size]; // small buffer
            };
            std::function<std::string(AnyWrapper const&, std::string_view)> to_string{nullptr};
            std::function<void(AnyWrapper&, std::pmr::polymorphic_allocator<std::byte>)> deleter{nullptr};
        };

        template <typename T>
        struct AnyHelper {
            static auto dealloc(AnyWrapper& wrapper, std::pmr::polymorphic_allocator<std::byte> alloc) -> void {
                if constexpr (sizeof(T) <= small_buffer_size) {
                    auto* tmp = reinterpret_cast<T*>(&wrapper.data);
                    tmp->~T();
                } else {
                    alloc.delete_object(reinterpret_cast<T*>(wrapper.ptr));
                }
            }
        };

        using allocator_t = std::pmr::polymorphic_allocator<std::byte>;
    public:

        constexpr FormatterAnyArg() noexcept = default;
        constexpr FormatterAnyArg(FormatterAnyArg const&) = delete;
        constexpr FormatterAnyArg(FormatterAnyArg && other) noexcept
            : m_wrapper(std::move(other.m_wrapper))
        {
            other.m_wrapper.deleter = nullptr;
        }
        constexpr FormatterAnyArg& operator=(FormatterAnyArg const&) = delete;
        constexpr FormatterAnyArg& operator=(FormatterAnyArg && other) noexcept {
            if (this == &other) return *this;
            auto temp = FormatterAnyArg(std::move(other));
            swap(*this, temp);
            return *this;
        }

        ~FormatterAnyArg() {
            if (m_wrapper.deleter) {
                m_wrapper.deleter(m_wrapper, m_alloc);
            }
        }

        template <IsFormattable T>
            requires (sizeof(T) <= small_buffer_size)
        constexpr FormatterAnyArg(
            T&& val,
            allocator_t alloc = std::pmr::get_default_resource()
        ) noexcept(std::is_nothrow_move_constructible_v<T>)
            : m_alloc(alloc)
        {
            m_wrapper = AnyWrapper {
                .data = {},
                .to_string = [](
                    AnyWrapper const& wrapper,
                    std::string_view fmt
                ) -> std::string {
                    using std::to_string;

                    auto* val = reinterpret_cast<T const*>(&wrapper.data);
                    if constexpr (IsFormattableUsingStandardFormat<T>) {
                        return std::vformat(fmt, std::make_format_args(*val));
                    } else if constexpr (
                        IsFormattableUsingGlobalOverload<T> ||
                        IsFormattableUsingStandardToString<T>
                    ) {
                        return to_string(*val);
                    } else if constexpr (IsFormattableUsingMember<T>) {
                        return val->to_string();
                    } else if constexpr (IsFormattableUsingStandardOStream<T>) {
                        std::stringstream os;
                        os << *val;
                        return os.str();
                    }
                },
                .deleter = AnyHelper<T>::dealloc
            };

            new(m_wrapper.data) T(std::move(val));
        }

        template <IsFormattable T>
        FormatterAnyArg(
            T&& val,
            allocator_t alloc = std::pmr::get_default_resource()
        )
            : m_alloc(alloc)
        {
            m_wrapper = AnyWrapper{
                .data = m_alloc.allocate(sizeof(T)),
                .to_string = [](
                    AnyWrapper const& wrapper,
                    std::string_view fmt
                ) -> std::string {
                    using std::to_string;

                    auto* val = reinterpret_cast<T const*>(wrapper.ptr);
                    if constexpr (IsFormattableUsingStandardFormat<T>) {
                        return std::vformat(fmt, std::make_format_args(*val));
                    } else if constexpr (
                        IsFormattableUsingGlobalOverload<T> ||
                        IsFormattableUsingStandardToString<T>
                    ) {
                        return to_string(*val);
                    } else if constexpr (IsFormattableUsingMember<T>) {
                        return val->to_string();
                    } else if constexpr (IsFormattableUsingStandardOStream<T>) {
                        std::stringstream os;
                        os << *val;
                        return os.str();
                    }
                },
                .deleter = AnyHelper<T>::dealloc
            };

            new(m_wrapper.data) T(std::move(val));
        }

        template <std::size_t N>
        FormatterAnyArg(const char(&val)[N]) noexcept {
            m_wrapper = AnyWrapper {
                .data = {},
                .to_string = [](
                    AnyWrapper const& wrapper,
                    [[maybe_unused]] std::string_view fmt
                ) -> std::string {
                    using std::to_string;

                    auto* val = reinterpret_cast<std::string_view const*>(&wrapper.data);
                    return std::string(*val);
                },
                .deleter = AnyHelper<std::string_view>::dealloc
            };

            new(m_wrapper.data) std::string_view(val);
        }

        FormatterAnyArg(std::string val) {
            m_wrapper = AnyWrapper {
                .data = {},
                .to_string = [](
                    AnyWrapper const& wrapper,
                    [[maybe_unused]] std::string_view fmt
                ) -> std::string {
                    using std::to_string;
                    auto val = reinterpret_cast<std::string const*>(&wrapper.data);
                    return *val;
                },
                .deleter = AnyHelper<std::string>::dealloc
            };

            new(m_wrapper.data) std::string(std::move(val));
        }

        auto to_string(std::string_view fmt) const -> std::string {
            if (m_wrapper.to_string) {
                return m_wrapper.to_string(m_wrapper, fmt); 
            } else {
                return "";
            }
        }

        constexpr friend auto swap(FormatterAnyArg& lhs, FormatterAnyArg& rhs) -> void {
            using std::swap;
            swap(lhs.m_wrapper, rhs.m_wrapper);
        }

        constexpr operator bool() const noexcept {
            return m_wrapper.to_string != nullptr;
        }
    private:
        std::pmr::polymorphic_allocator<std::byte> m_alloc;
        AnyWrapper m_wrapper{}; 
    };

} // namespace dark::core


namespace std {
    template<>
    struct formatter<::dark::core::FormatterAnyArg> {
        template<typename parse_context_t>
        constexpr auto parse(parse_context_t& ctx) -> typename parse_context_t::iterator {
            auto tmp = std::distance(ctx.begin(), ctx.end());
            m_format.reserve(static_cast<std::size_t>(tmp));

            auto it = ctx.begin();
            while (it != ctx.end()) {
                m_format += *it;
                if (*it == '}') {
                    break; 
                }
                ++it;
            }

            return it;
        }

        template<typename fmt_context_t>
        auto format(::dark::core::FormatterAnyArg const& val, fmt_context_t& ctx) const -> typename fmt_context_t::iterator {
            auto out = ctx.out();
            auto str = val.to_string(m_format);
            std::copy(str.begin(), str.end(), out);
            return out; 
        }

    private:
        std::string m_format{"{:"};
    };
} // namespace std

#endif // AMT_DARK_DIAGNOSTIC_CORE_FORMAT_ANY_HPP
