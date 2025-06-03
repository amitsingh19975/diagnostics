#ifndef AMT_DARK_DIAGANOSTICS_COW_STRING_HPP
#define AMT_DARK_DIAGANOSTICS_COW_STRING_HPP

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

namespace dark::core {

    struct CowString {
    private:
        static constexpr auto borrowed_index = std::size_t{0};
        static constexpr auto owned_index = std::size_t{1};
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

        struct OwnedTag{};
        struct BorrowedTag{};

        constexpr CowString() noexcept = default;
        CowString(CowString const& other)
            : m_data(other.to_owned())
        {}
        CowString(CowString&& other) noexcept {
            if (m_data.index() == borrowed_index) m_data = other.to_borrowed();
            else m_data = std::move(std::get<owned_index>(other.m_data));
        }
        CowString& operator=(CowString const& other) {
            if (this == &other) return *this;
            auto tmp = CowString(other);
            swap(*this, tmp);
            return *this;
        }
        CowString& operator=(CowString&& other) noexcept {
            if (this == &other) return *this;
            auto tmp = CowString(std::move(other));
            swap(*this, tmp);
            return *this;
        }
        ~CowString() = default;

        explicit CowString(char const* s, OwnedTag = {})
            : CowString(std::string(s))
        {}

        explicit CowString(char const* s, BorrowedTag)
            : CowString(std::string_view(s))
        {}

        explicit CowString(std::string const& s, OwnedTag = {})
            : m_data(s)
        {}

        explicit constexpr CowString(std::string&& s, OwnedTag = {}) noexcept
            : m_data(std::move(s))
        {}

        constexpr CowString(std::string_view s, BorrowedTag = {}) noexcept
            : m_data(s)
        {}

        template <std::size_t N>
        constexpr CowString(const char (&s)[N], OwnedTag) noexcept
            : m_data(std::string(s))
        {}

        template <std::size_t N>
        constexpr CowString(const char (&s)[N], BorrowedTag = {}) noexcept
            : m_data(std::string_view(s))
        {}

        constexpr auto is_owned() const noexcept -> bool { return m_data.index() == owned_index; }
        constexpr auto is_borrowed() const noexcept -> bool { return m_data.index() == borrowed_index; }

        constexpr auto empty() const noexcept -> bool {
            return std::visit([](auto const& s) { return s.empty(); }, m_data);
        }

        constexpr auto size() const noexcept -> size_type {
            return std::visit([](auto const& s) { return s.size(); }, m_data);
        }

        constexpr auto data() const noexcept -> const_pointer {
            return std::visit([](auto const& s) { return s.data(); }, m_data);
        }
        constexpr auto data() noexcept -> pointer {
            return std::visit([](auto& s) -> pointer { return const_cast<pointer>(s.data()); }, m_data);
        }

        constexpr auto begin() noexcept -> iterator { return data(); }
        constexpr auto end() noexcept -> iterator { return data() + size(); }
        constexpr auto begin() const noexcept -> const_iterator { return data(); }
        constexpr auto end() const noexcept -> const_iterator { return data() + size(); }

        constexpr auto rbegin() noexcept -> reverse_iterator {
            return std::reverse_iterator(end());
        }
        constexpr auto rend() noexcept -> reverse_iterator {
            return std::reverse_iterator(begin());
        }

        constexpr auto rbegin() const noexcept -> const_reverse_iterator {
            return std::reverse_iterator(end());
        }
        constexpr auto rend() const noexcept -> const_reverse_iterator {
            return std::reverse_iterator(begin());
        }

        constexpr auto to_borrowed() const noexcept -> std::string_view {
            return std::visit([](auto const& s) -> std::string_view { return s; }, m_data);
        }

        auto to_owned() const -> std::string {
            return std::string(to_borrowed());
        }

        auto conume() -> std::string {
            if (is_owned()) {
                return std::move(std::get<owned_index>(m_data));
            } else {
                return std::string(std::get<borrowed_index>(m_data));
            }
        }

        constexpr operator std::string_view() const noexcept {
            return to_borrowed();
        }

        constexpr friend auto swap(CowString& lhs, CowString& rhs) noexcept -> void {
            using std::swap;
            swap(lhs.m_data, rhs.m_data);
        }
    private:
        std::variant<std::string_view, std::string> m_data { std::string_view{} };
    };

} // dark::core

#endif // AMT_DARK_DIAGANOSTICS_COW_STRING_HPP
