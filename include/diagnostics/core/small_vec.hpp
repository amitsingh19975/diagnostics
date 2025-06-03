#ifndef AMT_DARK_DIAGNOSTIC_CORE_SMALL_VEC_HPP
#define AMT_DARK_DIAGNOSTIC_CORE_SMALL_VEC_HPP

#include <cassert>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <bit>
#include <algorithm>

namespace dark::core {

    template <typename T, std::size_t N>
    struct Array {
    private:
        static constexpr auto type_size = sizeof(T);
        struct TypeWrapper {
            alignas(alignof(T)) std::byte value[type_size]; 
        };
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using pointer = T*;
        using const_pointer = T const*;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        constexpr Array() noexcept = default;
        constexpr Array(Array const& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
            : m_size(other.m_size)
        {
            if constexpr (std::is_trivially_copy_constructible_v<T>) {
                std::copy_n(other.m_data, m_size, m_data);
            } else {
                std::copy(other.begin(), other.end(), begin());
            }
        }

        constexpr Array(Array&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            : m_size(other.m_size)
        {
            if constexpr (std::is_trivially_move_constructible_v<T>) {
                std::copy_n(other.m_data, m_size, m_data);
            } else {
                std::move(other.begin(), other.end(), begin());
            }
            other.m_size = 0;
        }
        constexpr Array& operator=(Array const& other) noexcept(std::is_nothrow_move_assignable_v<T>) {
            if (this == &other) return *this; 
            m_size = other.m_size;
            if constexpr (std::is_trivially_copy_constructible_v<T>) {
                std::copy_n(other.m_data, m_size, m_data);
            } else {
                std::copy(other.begin(), other.end(), begin());
            }
            return *this;
        }
        constexpr Array& operator=(Array&& other) noexcept(std::is_nothrow_move_assignable_v<T>) {
            if (this == &other) return *this; 
            if constexpr (std::is_trivially_move_constructible_v<T>) {
                std::copy_n(other.m_data, m_size, m_data);
            } else {
                std::move(other.begin(), other.end(), begin());
            }
            m_size = std::exchange(other.m_size, 0);
            return *this;
        }
        constexpr ~Array() noexcept requires (std::is_trivially_destructible_v<T>) = default;
        ~Array() noexcept(std::is_nothrow_destructible_v<T>) requires (!std::is_trivially_destructible_v<T>) {
            for (auto& el: *this) el.~T();
        }

        constexpr Array(const T (&data)[N]) noexcept(std::is_nothrow_copy_constructible_v<T> || std::is_trivially_copy_assignable_v<T>)
            : m_size(N)
        {
            for (auto i = 0ul; i < N; ++i) {
                if constexpr (std::is_trivially_copy_assignable_v<T>) {
                    m_data[i] = std::bit_cast<TypeWrapper>(data[i]);
                } else {
                    new(m_data + i) T(data[i]);
                }
            }
        }

        constexpr Array(std::initializer_list<T> li) noexcept(std::is_nothrow_copy_constructible_v<T> || std::is_trivially_copy_assignable_v<T>)
            : m_size(li.size())
        {
            for (auto i = 0ul; auto&& el: li) {
                if constexpr (std::is_trivially_copy_assignable_v<T>) {
                    m_data[i] = std::bit_cast<TypeWrapper>(std::move(el));
                } else {
                    new(m_data + i) T(std::move(el));
                }
                ++i;
            }
        }

        constexpr Array(size_type n) noexcept(std::is_nothrow_constructible_v<T>) requires (std::is_default_constructible_v<T>)
            : m_size(n)
        {
            assert(n < N);
            for (auto i = 0ul; i < N; i += sizeof(T)) {
                if constexpr (!std::is_trivially_constructible_v<T>) {
                    new(m_data + i) T();
                }
            }
        }

        constexpr Array(size_type n, T const& def) noexcept(std::is_nothrow_copy_constructible_v<T> || std::is_trivially_copy_assignable_v<T>)
            : m_size(n)
        {
            assert(n < N);
            for (auto i = 0ul; i < n; ++i) {
                if constexpr (std::is_trivially_copy_assignable_v<T>) {
                    m_data[i] = std::bit_cast<TypeWrapper>(def);
                } else {
                    new(m_data + i) T(def);
                }
            }
        }

        constexpr auto size() const noexcept -> size_type { return m_size; }
        constexpr auto empty() const noexcept -> bool { return size() == 0; }
        constexpr auto full() const noexcept -> bool { return size() == N; }

        auto data() noexcept -> pointer {
            return reinterpret_cast<T*>(m_data);
        }

        auto data() const noexcept -> const_pointer {
            return reinterpret_cast<T const*>(m_data);
        }

        auto operator[](size_type k) const noexcept -> const_reference {
            return data()[k];
        }

        auto operator[](size_type k) noexcept -> reference {
            return data()[k];
        }

        auto begin() noexcept -> iterator { return data(); }
        auto end() noexcept -> iterator { return data() + size(); }
        auto begin() const noexcept -> const_iterator { return data(); }
        auto end() const noexcept -> const_iterator { return data() + size(); }

        auto rbegin() noexcept -> reverse_iterator { return reverse_iterator(begin()); }
        auto rend() noexcept -> reverse_iterator { return reverse_iterator(end()); }
        auto rbegin() const noexcept -> const_reverse_iterator { return reverse_iterator(begin()); }
        auto rend() const noexcept -> const_reverse_iterator { return reverse_iterator(end()); }

        auto push_back(const_reference val) noexcept(std::is_nothrow_copy_assignable_v<T>) {
            assert((m_size < N) && "array is full");
            if constexpr (std::is_trivially_copy_assignable_v<T>) {
                m_data[m_size] = std::bit_cast<TypeWrapper>(val);
            } else {
                new (m_data + m_size) T(val);
            }
            ++m_size;
        }

        auto push_back(value_type&& val) noexcept(std::is_nothrow_move_assignable_v<T>) {
            assert((m_size < N) && "array is full");
            if constexpr (std::is_trivially_copy_assignable_v<T>) {
                m_data[m_size] = std::bit_cast<TypeWrapper>(val);
            } else {
                new (m_data + m_size) T(std::move(val));
            }
            ++m_size;
        }

        template <typename... Args>
        auto emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
            assert((m_size < N) && "array is full");
            new (m_data + m_size++) T(std::forward<Args>(args)...);
        }

        auto pop_back() noexcept -> void {
            assert(!empty());
            data()[--m_size].~T();
        }

        auto back() noexcept -> reference {
            assert(!empty());
            return data()[m_size - 1];
        }

        auto back() const noexcept -> const_reference {
            assert(!empty());
            return data()[m_size - 1];
        }

        auto front() noexcept -> reference {
            assert(!empty());
            return data()[0];
        }

        auto front() const noexcept -> const_reference {
            assert(!empty());
            return data()[0];
        }

        auto resize(size_type n) noexcept(std::is_nothrow_copy_assignable_v<T>) requires (std::is_default_constructible_v<T>) {
            assert(n <= N && "cannot go beyond the capacity");
            while (size() > n) pop_back();
            if constexpr (std::is_trivially_copy_assignable_v<T>) {
                m_size = n;
            } else {
                while (size() < n) push_back(T());
            }
        }

        auto resize(size_type n, T const& def) noexcept(std::is_nothrow_copy_assignable_v<T>) {
            assert(n <= N && "cannot go beyond the capacity");
            while (size() > n) pop_back();
            while (size() < n) push_back(def);
        }

        auto clear() noexcept(std::is_nothrow_destructible_v<T>) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (auto& el: *this) el.~T();
            }

            m_size = 0;
        }

        auto erase(iterator pos) noexcept (std::is_nothrow_move_assignable_v<T>) {
            if (empty()) return;
            std::move(pos + 1, end(), pos);
            m_size -= 1;
        }

        auto erase(iterator first, iterator last) noexcept(std::is_nothrow_move_assignable_v<T>) -> void {
            auto diff = static_cast<size_type>(last - first);
            if (diff >= size()) {
                clear();
                return;
            }
            std::move(last, end(), first);
            m_size -= diff;
        }

        friend auto swap(Array& lhs, Array& rhs) noexcept -> void {
            using std::swap;
            swap(lhs.m_size, rhs.m_size);
            for (auto i = 0ul; i < N; ++i) {
                swap(lhs[i], rhs[i]);
            }
        }

        auto operator==(Array const& other) const noexcept -> bool {
            return std::equal(begin(), end(), other.begin());
        }

    private:
        TypeWrapper m_data[N];
        size_type m_size{}; 
    };

    // TODO: remove variant altogether and replace it union of T* and T[]
    template <typename T, unsigned Cap = 8, typename A = std::allocator<T>>
    struct SmallVec {
        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using pointer = T*;
        using const_pointer = T const*;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using allocator_t = A; 
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
    private:
        using array_t = Array<T, Cap>;
        using vec_t = std::vector<T, A>;
        using base_type = std::variant<array_t, vec_t>;
        static constexpr size_type array_index = 0; 
        static constexpr size_type vec_index = 1;
    public:

        SmallVec() noexcept
            : m_data(array_t{})
        {}

        SmallVec(SmallVec const& other) {
            if (is_small()) m_data = std::get<array_index>(other.m_data);
            else m_data = std::get<vec_index>(other.m_data);
        }
        SmallVec(SmallVec&& other) noexcept {
            if (is_small()) m_data = std::move(std::get<array_index>(other.m_data));
            else m_data = std::move(std::get<vec_index>(other.m_data));
        }
        SmallVec& operator=(SmallVec const& other) {
            if (this == &other) return *this;
            auto tmp = SmallVec(other);
            swap(tmp, *this);
            return *this;
        }
        SmallVec& operator=(SmallVec&& other) noexcept {
            if (this == &other) return *this;
            auto tmp = SmallVec(std::move(other));
            swap(tmp, *this);
            return *this;
        }
        ~SmallVec() = default;

        SmallVec(std::initializer_list<T> li) {
            if (li.size() > Cap) m_data = vec_t(li);
            else m_data = array_t(li);
        }

        SmallVec(size_type n)
            : SmallVec()
        {
            if (n > Cap) m_data = vec_t(n);
            else m_data = array_t(n);
        }

        SmallVec(size_type n, T def) {
            if (n > Cap) m_data = vec_t(n, def);
            else m_data = array_t(n, def);
        }

        constexpr auto size() const noexcept -> size_type {
            return std::visit([](auto const& v) -> size_type { return v.size(); }, m_data);
        }

        constexpr auto empty() const noexcept -> bool {
            return size() == 0;
        }

        template <typename U>
            requires (std::same_as<std::remove_cvref_t<std::decay_t<U>>, T>)
        auto push_back(U&& val) -> void {
            if (is_small()) {
                array_t& tmp = std::get<array_index>(m_data);
                if (!tmp.full()) {
                    tmp.push_back(std::forward<U>(val));
                } else {
                    auto v = vec_t();
                    v.reserve(size() + 1);
                    std::move(tmp.begin(), tmp.end(), std::back_inserter(v));
                    v.push_back(std::forward<U>(val));
                    m_data = std::move(v);
                }
            } else {
                vec_t& tmp = std::get<vec_index>(m_data);
                tmp.push_back(std::forward<U>(val));
            }
        }

        auto pop_back() -> void {
            if (empty()) return;

            if (is_small()) {
                array_t& tmp = std::get<array_index>(m_data);
                tmp.pop_back();
            } else {
                vec_t& tmp = std::get<vec_index>(m_data);
                tmp.pop_back();
            }
        }

        template <typename... Args>
            requires std::constructible_from<T, Args...>
        auto emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> void {
            if (is_small()) {
                array_t& tmp = std::get<array_index>(m_data);
                if (size() + 1 <= Cap) {
                    tmp.emplace_back(std::forward<Args>(args)...);
                } else {
                    auto v = vec_t{};
                    v.reserve(size() + 1);
                    std::move(tmp.begin(), tmp.end(), std::back_inserter(v));
                    v.emplace_back(std::forward<Args>(args)...);
                    m_data = std::move(v);
                }
            } else {
                vec_t& tmp = std::get<vec_index>(m_data);
                tmp.emplace_back(std::forward<Args>(args)...);
            }
        }


        auto back() -> reference {
            assert(!empty());
            if (is_small()) {
                auto& tmp = std::get<array_index>(m_data);
                return tmp.back();
            } else {
                auto& tmp = std::get<vec_index>(m_data);
                return tmp.back();
            }
        }

        auto back() const -> const_reference {
            auto self = const_cast<SmallVec*>(this);
            return self->back();
        }


        auto front() -> reference {
            assert(!empty());
            if (is_small()) {
                auto& tmp = std::get<array_index>(m_data);
                return tmp.front();
            } else {
                auto& tmp = std::get<vec_index>(m_data);
                return tmp.front();
            }
        }

        auto front() const -> const_reference {
            auto self = const_cast<SmallVec*>(this);
            return self->front();
        }

        constexpr auto data() noexcept -> pointer {
            return std::visit([](auto& v) { return v.data(); }, m_data);
        }

        constexpr auto data() const noexcept -> const_pointer {
            auto& self = *const_cast<SmallVec*>(this);
            return self.data();
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

        constexpr auto is_small() const noexcept -> bool {
            return m_data.index() == array_index;
        }

        constexpr operator std::span<T>() const noexcept {
            auto self = const_cast<SmallVec*>(this);
            return { self->data(), size() };
        }

        constexpr auto operator[](size_type k) noexcept -> reference {
            assert(k < size());
            return data()[k];
        }
        constexpr auto operator[](size_type k) const noexcept -> const_reference {
            assert(k < size());
            return data()[k];
        }

        constexpr auto contains(T const& needle) const noexcept -> bool requires (std::equality_comparable<T>) {
            for (auto const& el: *this) {
                if (el == needle) return true;
            }
            return false;
        }

        auto reserve(size_type n) {
            if (n > Cap) {
                if (m_data.index() == vec_index) {
                    auto& tmp = std::get<vec_index>(m_data);
                    tmp.reserve(n);
                } else {
                    auto v = vec_t{};
                    v.reserve(std::max(n, size()));
                    std::move(begin(), end(), v.begin());
                    m_data = std::move(v);
                }
            }
        }

        auto resize(size_type n) {
            if (n > Cap) {
                if (m_data.index() == vec_index) {
                    auto& tmp = std::get<vec_index>(m_data);
                    tmp.resize(n);
                } else {
                    auto v = vec_t{};
                    v.resize(n);
                    auto d = data();
                    auto sz = std::min(size(), n);
                    for (auto i = 0ul; i < sz; ++i) {
                        v[i] = std::move(d[i]);
                    }

                    m_data = std::move(v);
                }
            } else {
                std::visit([n](auto& v) { v.resize(n); }, m_data);
            }
        }

        auto resize(size_type n, T const& def) {
            if (n > Cap) {
                if (m_data.index() == vec_index) {
                    auto& tmp = std::get<vec_index>(m_data);
                    tmp.resize(n, def);
                } else {
                    auto v = vec_t{};
                    v.reserve(n);
                    auto d = data();
                    auto sz = std::min(size(), n);
                    for (auto i = 0ul; i < sz; ++i) {
                        v.push_back(std::move(d[i]));
                    }

                    m_data = std::move(v);
                }
            } else {
                std::visit([n, &def](auto& v) { v.resize(n, def); }, m_data);
            }
        }

        auto clear() noexcept(std::is_nothrow_destructible_v<T>) {
            std::visit([](auto& v) { v.clear(); }, m_data);
        }

        auto erase(iterator pos) -> void {
            if (is_small()) {
                array_t& tmp = std::get<array_index>(m_data);
                tmp.erase(pos);
            } else {
                vec_t& tmp = std::get<vec_index>(m_data);
                auto it = tmp.begin() + static_cast<vec_t::difference_type>(pos - begin());
                tmp.erase(it);
            }
        }

        auto erase(iterator first, iterator last) -> void {
            if (is_small()) {
                array_t& tmp = std::get<array_index>(m_data);
                tmp.erase(first, last);
            } else {
                vec_t& tmp = std::get<vec_index>(m_data);
                auto f = tmp.begin() + static_cast<vec_t::difference_type>(first - begin());
                auto l = tmp.begin() + static_cast<vec_t::difference_type>(last - begin());
                tmp.erase(f, l);
            }
        }

        friend auto swap(SmallVec& lhs, SmallVec& rhs) noexcept -> void {
            using std::swap;
            swap(lhs.m_data, rhs.m_data);
        }

        constexpr auto operator==(SmallVec const& other) const noexcept -> bool {
            return m_data == other.m_data;
        }

    private:
        base_type m_data;
    };

    template <typename T, typename A>
    struct SmallVec<T, 0, A> {
        using base_type = std::vector<T, A>;
        using value_type = typename base_type::value_type;
        using reference = typename base_type::reference;
        using const_reference = typename base_type::const_reference;
        using pointer = typename base_type::pointer;
        using const_pointer = typename base_type::const_pointer;
        using iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;
        using reverse_iterator = typename base_type::reverse_iterator;
        using const_reverse_iterator = typename base_type::const_reverse_iterator;
        using allocator_t = A; 
        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;

        SmallVec() noexcept = default;

        SmallVec(SmallVec const& other) = default;
        SmallVec(SmallVec&& other) noexcept = default;
        SmallVec& operator=(SmallVec const& other) = default;
        SmallVec& operator=(SmallVec&& other) noexcept = default;
        ~SmallVec() = default;

        SmallVec(std::initializer_list<T> li)
            : m_data(li)
        {}

        SmallVec(size_type n)
            : m_data(n)
        {}

        SmallVec(size_type n, T def)
            : m_data(n, std::move(def))
        {}

        constexpr auto size() const noexcept -> size_type {
            return m_data.size();
        }

        constexpr auto empty() const noexcept -> bool {
            return m_data.empty();
        }

        auto push_back(const_reference val) -> void {
            m_data.push_back(val);
        }

        auto push_back(value_type&& val) -> void {
            m_data.push_back(val);
        }

        auto pop_back() -> void {
            assert(!empty());
            m_data.pop_back();
        }

        template <typename... Args>
            requires std::constructible_from<T, Args...>
        auto emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> void {
            m_data.emplace_back(std::forward<Args>(args)...);
        }


        auto back() -> reference {
            assert(!empty());
            return m_data.back();
        }

        auto back() const -> const_reference {
            assert(!empty());
            return m_data.back();
        }


        auto front() -> reference {
            assert(!empty());
            return m_data.front();
        }

        auto front() const -> const_reference {
            assert(!empty());
            return m_data.front();
        }

        constexpr auto data() noexcept -> pointer {
            return m_data.data();
        }

        constexpr auto data() const noexcept -> const_pointer {
            return m_data.data();
        }

        constexpr auto begin() noexcept -> iterator { return m_data.begin(); }
        constexpr auto end() noexcept -> iterator { return m_data.end(); }

        constexpr auto begin() const noexcept -> const_iterator { return m_data.begin(); }
        constexpr auto end() const noexcept -> const_iterator { return m_data.end(); }

        constexpr auto rbegin() noexcept -> reverse_iterator {
            return m_data.rbegin();
        }
        constexpr auto rend() noexcept -> reverse_iterator {
            return m_data.rbegin();
        }

        constexpr auto rbegin() const noexcept -> const_reverse_iterator {
            return m_data.rbegin();
        }
        constexpr auto rend() const noexcept -> const_reverse_iterator {
            return m_data.rend();
        }

        constexpr auto is_small() const noexcept -> bool {
            return false;
        }

        constexpr operator std::span<T>() const noexcept {
            return std::span(m_data);
        }

        constexpr auto operator[](size_type k) noexcept -> reference {
            assert(k < size());
            return m_data[k];
        }
        constexpr auto operator[](size_type k) const noexcept -> const_reference {
            assert(k < size());
            return m_data[k];
        }

        constexpr auto contains(T const& needle) const noexcept -> bool requires (std::equality_comparable<T>) {
            for (auto const& el: *this) {
                if (el == needle) return true;
            }
            return false;
        }

        auto reserve(size_type n) {
            m_data.reserve(n);
        }

        auto resize(size_type n) {
            m_data.resize(n);
        }

        auto resize(size_type n, T const& def) {
            m_data.resize(n, def);
        }

        auto clear() noexcept(std::is_nothrow_destructible_v<T>) {
            m_data.clear();
        }

        auto erase(iterator pos) -> void {
            m_data.erase(pos);
        }

        auto erase(iterator first, iterator last) -> void {
            m_data.erase(first, last);
        }

        friend auto swap(SmallVec& lhs, SmallVec& rhs) noexcept -> void {
            using std::swap;
            swap(lhs.m_data, rhs.m_data);
        }

        constexpr auto operator==(SmallVec const& other) const noexcept -> bool {
            return m_data == other.m_data;
        }

    private:
        base_type m_data;
    };
} // namespace dark::core

#endif // AMT_DARK_DIAGNOSTIC_CORE_SMALL_VEC_HPP
