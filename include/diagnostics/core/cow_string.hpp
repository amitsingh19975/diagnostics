#ifndef AMT_DARK_DIAGANOSTICS_COW_STRING_HPP
#define AMT_DARK_DIAGANOSTICS_COW_STRING_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

namespace dark::core {

    struct CowString {
    private:
        struct Wrapper {
            alignas(std::max(alignof(std::string_view), alignof(std::string))) 
            std::byte data[std::max(sizeof(std::string_view), sizeof(std::string))];
        };
        enum State: std::uint8_t {
            NONE,
            OWNED,
            BORROWED
        };
    public:
        using size_type = std::size_t;
        using reference = char&;
        using const_reference = char const&;
        using pointer = char*;
        using const_pointer = char const*;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using difference_type = std::ptrdiff_t;
        static constexpr auto npos = std::string_view::npos;

        struct OwnedTag{};
        struct BorrowedTag{};

        constexpr CowString() noexcept {}
        CowString(CowString const& other)
            : CowString(other.to_owned())
        {}
        CowString(CowString&& other) noexcept
            : m_data(other.m_data)
            , m_state(std::exchange(other.m_state, NONE))
        {}
        CowString& operator=(CowString const& other) {
            if (this == &other) return *this;
            auto tmp = CowString(other);
            std::swap(m_data, tmp.m_data);
            m_state = std::exchange(tmp.m_state, NONE);
            return *this;
        }
        CowString& operator=(CowString&& other) noexcept
        {
            if (this == &other) return *this;
            m_data = std::move(other.m_data);
            m_state = std::exchange(other.m_state, NONE);
            return *this;
        }
        ~CowString() {
            destroy();
        }

        explicit CowString(char const* s, OwnedTag = {})
            : CowString(std::string(s))
        {}

        explicit CowString(char const* s, BorrowedTag)
            : CowString(std::string_view(s))
        {}

        explicit CowString(std::string const& s, OwnedTag = {})
            : m_state(OWNED)
        {
            new(m_data.data) std::string(s);
        }

        explicit constexpr CowString(std::string&& s, OwnedTag = {}) noexcept
            : m_state(OWNED)
        {
            new(m_data.data) std::string(s);
        }

        constexpr CowString(std::string_view s, BorrowedTag = {}) noexcept
            : m_state(BORROWED)
        {
            new(m_data.data) std::string_view(s);
        }

        template <std::size_t N>
        constexpr CowString(const char (&s)[N], OwnedTag) noexcept
            : m_state(OWNED)
        {
            new(m_data.data) std::string(s);
        }

        template <std::size_t N>
        constexpr CowString(const char (&s)[N], BorrowedTag = {}) noexcept
            : m_state(BORROWED)
        {
            new(m_data.data) std::string_view(s);
        }

        constexpr auto is_owned() const noexcept -> bool {
            return m_state == OWNED;
        }
        constexpr auto is_borrowed() const noexcept -> bool {
            return m_state == BORROWED;
        }

        constexpr auto empty() const noexcept -> bool {
            if (is_owned()) return as_owned().empty();
            else return as_borrowed().empty();
        }

        constexpr auto size() const noexcept -> size_type {
            if (is_owned()) return as_owned().size();
            else return as_borrowed().size();
        }

        constexpr auto data() const noexcept -> const_pointer {
            if (is_owned()) return as_owned().data();
            else return as_borrowed().data();
        }

        constexpr auto begin() const noexcept -> const_iterator { return data(); }
        constexpr auto end() const noexcept -> const_iterator { return data() + size(); }

        constexpr auto rbegin() const noexcept -> const_reverse_iterator {
            return std::reverse_iterator(end());
        }
        constexpr auto rend() const noexcept -> const_reverse_iterator {
            return std::reverse_iterator(begin());
        }

        constexpr auto to_borrowed() const noexcept -> std::string_view {
            if (is_owned()) return as_owned();
            else return as_borrowed();
        }

        auto to_owned() const -> std::string {
            return std::string(to_borrowed());
        }

        auto consume() -> std::string {
            m_state = NONE;
            if (is_owned()) {
                return std::string(std::move(as_owned()));
            } else {
                return std::string(as_borrowed());
            }
        }

        constexpr operator std::string_view() const noexcept {
            return to_borrowed();
        }

        constexpr auto find(std::string_view sep) const noexcept -> size_type {
            return to_borrowed().find(sep);
        }

        auto substr(size_type pos = 0, size_type n = npos) const -> CowString {
            if (is_owned()) return CowString(as_owned().substr(pos, n));
            else return CowString(as_borrowed().substr(pos, n));
        }
    private:
        auto as_owned() -> std::string& {
            assert(is_owned());
            return *reinterpret_cast<std::string*>(m_data.data);
        }

        auto as_owned() const -> std::string const& {
            assert(is_owned());
            return *reinterpret_cast<std::string const*>(m_data.data);
        }

        auto as_borrowed() const -> std::string_view {
            assert(is_borrowed());
            return *reinterpret_cast<std::string_view const*>(m_data.data);
        }

        auto destroy() -> void {
            using namespace std;
            if (is_owned()) as_owned().~string();
            else if (is_borrowed()) as_borrowed().~string_view();
            m_state = NONE;
        }
    private:
        Wrapper m_data;
        State m_state{NONE};
    };

} // dark::core

#endif // AMT_DARK_DIAGANOSTICS_COW_STRING_HPP
