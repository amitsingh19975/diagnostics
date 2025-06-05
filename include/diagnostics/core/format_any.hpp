#ifndef AMT_DARK_DIAGNOSTIC_CORE_FORMAT_ANY_HPP
#define AMT_DARK_DIAGNOSTIC_CORE_FORMAT_ANY_HPP

#include "cow_string.hpp"
#include <concepts>
#include <cstddef>
#include <format>
#include <memory_resource>
#include <sstream>
#include <string>
#include <memory.h>
#include <string_view>
#include <utility>

namespace dark::core {

    template <typename T>
    concept IsFormattableUsingGlobalOverload = requires (T&& t) {
        { to_string(t) } -> std::convertible_to<std::string_view>;
    };

    template <typename T>
    concept IsFormattableUsingStandardToString = requires (T&& t) {
        { std::to_string(t) } -> std::convertible_to<std::string_view>;
    };

    template <typename T>
    concept IsFormattableUsingMember = requires (T&& t) {
        { t.to_string() } -> std::convertible_to<std::string_view>;
    };

    namespace detail {
        template <template <typename... InnerArgs> class Tmpl>
        struct IsSpecialized {
        private:
            template <typename... Args, class dummy = decltype(Tmpl<Args...>{})>
            static constexpr auto exists(int) noexcept -> bool {
                return true;
            }

            template <typename... Args>
            static constexpr auto exists(char) noexcept -> bool {
                return false;
            }

        public:
           template <typename... Args>
           static constexpr auto check() noexcept -> bool {
                return exists<Args...>(42);
           }
        };
    } // namespace detail

    template <typename T, typename CharT = char>
    concept IsFormattableUsingStandardFormat = detail::IsSpecialized<std::formatter>::check<T, CharT>();

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
            sizeof(std::string) + alignof(std::string),
            sizeof(CowString) + alignof(CowString)
        });
    private:
        using allocator_t = std::pmr::polymorphic_allocator<std::byte>;
        struct AnyWrapper {
            union {
                std::byte* ptr{nullptr};
                std::byte buf[small_buffer_size]; // small buffer
            } data;
            std::string(*to_string)(AnyWrapper const&, std::string_view){nullptr};
            void(*deleter)(AnyWrapper&, allocator_t&){nullptr};

            constexpr AnyWrapper() noexcept = default;
            constexpr AnyWrapper(std::byte* ptr) noexcept
                : data(ptr)
            {}

            AnyWrapper(AnyWrapper const& other) = delete;
            constexpr AnyWrapper(AnyWrapper && other) noexcept
                : data(other.data)
                , to_string(std::exchange(other.to_string, nullptr))
                , deleter(std::exchange(other.deleter, nullptr))
            {}
            AnyWrapper& operator=(AnyWrapper const& other) = delete;
            constexpr AnyWrapper& operator=(AnyWrapper && other) noexcept {
                if (this == &other) return *this;
                data = other.data;
                to_string = std::exchange(other.to_string, nullptr);
                deleter = std::exchange(other.deleter, nullptr);
                return *this;
            }

            template <typename T>
            auto get() noexcept -> T* {
                if constexpr (sizeof(T) <= small_buffer_size) {
                    // small data
                    return reinterpret_cast<T*>(data.buf);
                } else {
                    // big data
                    return reinterpret_cast<T*>(data.ptr);
                }
            }

            template <typename T>
            auto get() const noexcept -> T const* {
                if constexpr (sizeof(T) <= small_buffer_size) {
                    // small data
                    return reinterpret_cast<T const*>(data.buf);
                } else {
                    // big data
                    return reinterpret_cast<T const*>(data.ptr);
                }
            }

            template <typename T>
            auto mem_ptr() noexcept -> T*& {
                return *reinterpret_cast<T**>(data);
            }

            auto dealloc(allocator_t& alloc) -> void {
                if (!deleter) {
                    deleter(*this, alloc);
                }
            }
        };

        template <typename T>
        struct AnyHelper {
            static auto dealloc(AnyWrapper& wrapper, allocator_t& alloc) -> void {
                if constexpr (sizeof(T) <= small_buffer_size) {
                    wrapper.get<T>()->~T();
                } else {
                    alloc.delete_object(wrapper.get<T>());
                }
            }

            static auto to_string(AnyWrapper const& wrapper, std::string_view fmt) -> std::string {
                using std::to_string;

                auto* val = wrapper.get<T>();
                if constexpr (IsFormattableUsingStandardFormat<T>) {
                    return std::vformat(fmt, std::make_format_args(*val));
                } else if constexpr (
                    IsFormattableUsingGlobalOverload<T> ||
                    IsFormattableUsingStandardToString<T>
                ) {
                    return std::string(to_string(*val));
                } else if constexpr (IsFormattableUsingMember<T>) {
                    return std::string(val->to_string());
                } else if constexpr (IsFormattableUsingStandardOStream<T>) {
                    std::stringstream os;
                    os << *val;
                    return os.str();
                }
            }
        };

    public:

        constexpr FormatterAnyArg() noexcept = default;
        constexpr FormatterAnyArg(FormatterAnyArg const&) = delete;
        constexpr FormatterAnyArg(FormatterAnyArg && other) noexcept
            : m_wrapper(std::move(other.m_wrapper))
        {}
        constexpr FormatterAnyArg& operator=(FormatterAnyArg const&) = delete;
        constexpr FormatterAnyArg& operator=(FormatterAnyArg && other) noexcept {
            if (this == &other) return *this;
            m_wrapper = std::move(other.m_wrapper);
            return *this;
        }

        ~FormatterAnyArg() {
            m_wrapper.dealloc(m_alloc);
        }

        template <IsFormattable T>
            requires (sizeof(T) <= small_buffer_size)
        constexpr FormatterAnyArg(
            T&& val,
            allocator_t alloc = std::pmr::get_default_resource()
        ) noexcept(std::is_nothrow_move_constructible_v<T>)
            : m_alloc(alloc)
        {
            using type = std::remove_cvref_t<T>;
            m_wrapper.to_string = AnyHelper<type>::to_string;
            m_wrapper.deleter = AnyHelper<type>::dealloc;
            new(m_wrapper.data.buf) type(std::move(val));
        }

        template <IsFormattable T>
        FormatterAnyArg(
            T&& val,
            allocator_t alloc = std::pmr::get_default_resource()
        )
            : m_alloc(alloc)
        {
            using type = std::remove_cvref_t<T>;
            m_wrapper.data.ptr = m_alloc.allocate(sizeof(T));
            m_wrapper.to_string = AnyHelper<type>::to_string;
            m_wrapper.deleter = AnyHelper<type>::dealloc;
            new(m_wrapper.data.ptr) type(std::move(val));
        }

        FormatterAnyArg(
            CowString&& val,
            allocator_t alloc = std::pmr::get_default_resource()
        )
            : m_alloc(alloc)
        {
            m_wrapper.to_string = +[](
                AnyWrapper const& wrapper,
                std::string_view
            ) {
                return wrapper.get<CowString>()->to_owned();
            };
            m_wrapper.deleter = AnyHelper<CowString>::dealloc;
            new(m_wrapper.data.buf) CowString(std::move(val));
        }

        FormatterAnyArg(
            CowString const& val,
            allocator_t alloc = std::pmr::get_default_resource()
        )
           : FormatterAnyArg(CowString(val), alloc)
        {}

        auto to_string(std::string_view fmt) const -> std::string {
            if (m_wrapper.to_string) {
                return m_wrapper.to_string(m_wrapper, fmt); 
            } else {
                return "";
            }
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
