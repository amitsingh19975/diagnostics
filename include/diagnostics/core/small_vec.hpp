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
#include <vector>
#include <algorithm>

namespace dark::core {

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
        using alloc_trait_t = std::allocator_traits<A>; 
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
    private:
        static constexpr auto type_size = sizeof(T);
        struct TypeWrapper {
            std::byte value[type_size]; 
        };
        union Wrapper {
            alignas(alignof(T)) TypeWrapper static_[Cap];
            TypeWrapper* dyn;
        };
        static constexpr auto growth_factor = size_type{2};
    public:

        SmallVec() noexcept = default;

        SmallVec(SmallVec const& other)
            : m_alloc(other.m_alloc)
        {
            resize(other.size());
            std::copy(other.begin(), other.end(), begin());
        }
        SmallVec(SmallVec&& other) noexcept
            : m_alloc(other.m_alloc)
            , m_size(std::exchange(other.m_size, 0))
            , m_capacity(std::exchange(other.m_capacity, 0))
        {
            if (other.is_small()) {
                auto lhs = data();
                auto rhs = other.data();
                std::move(rhs, rhs + size(), lhs);
            } else {
                m_data.dyn = std::exchange(other.m_data.dyn, nullptr); 
            }
        }
        SmallVec& operator=(SmallVec const& other) {
            if (this == &other) return *this;
            resize(other.size());
            std::copy(other.begin(), other.end(), begin());
            return *this;
        }
        SmallVec& operator=(SmallVec&& other) noexcept {
            if (this == &other) return *this;
            m_size = other.m_size;
            m_capacity = other.m_capacity;

            if (other.is_small()) {
                std::move(other.begin(), other.end(), begin());
            } else {
                m_data.dyn = std::exchange(other.m_data.dyn, nullptr); 
            }
            other.m_size = 0;
            other.m_capacity = 0;
            return *this;
        }
        ~SmallVec() {
            for (auto& el: *this) el.~T();

            if (!is_small() && m_data.dyn && capacity() > 0) {
                m_alloc.deallocate(reinterpret_cast<pointer>(m_data.dyn), capacity());
            }
        }

        SmallVec(std::initializer_list<T> li)
            : SmallVec(li.size())
        {
            std::move(li.begin(), li.end(), begin());
        }

        SmallVec(std::span<T> li)
            : SmallVec(li.size())
        {
            std::move(li.begin(), li.end(), begin());
        }

        SmallVec(size_type n) {
            resize(n);
        }

        SmallVec(size_type n, T def) {
            resize(n, def);
        }

        template <std::size_t OCap>
        SmallVec(SmallVec<T, OCap> const& other)
            : SmallVec(other.size())
        {
            std::copy(other.begin(), other.end(), begin());
        }

        template <std::size_t OCap>
        SmallVec(SmallVec<T, OCap> && other)
            : SmallVec(other.size())
        {
            std::move(other.begin(), other.end(), begin());
        }

        constexpr auto size() const noexcept -> size_type {
            return m_size;
        }

        constexpr auto capacity() const noexcept -> size_type {
            return m_capacity;
        }

        constexpr auto empty() const noexcept -> bool {
            return size() == 0;
        }

        auto push_back(const_reference val) -> void {
            reserve(size() + 1);
            alloc_trait_t::construct(m_alloc, data() + m_size++, val);
        }

        auto push_back(value_type&& val) -> void {
            reserve(size() + 1);

            alloc_trait_t::construct(m_alloc, data() + m_size++, std::move(val));
        }

        auto pop_back() -> void {
            if (empty()) return;
            auto& tmp = data()[--m_size];
            tmp.~T();
        }

        template <typename... Args>
            requires std::constructible_from<T, Args...>
        auto emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> void {
            reserve(size() + 1);
            alloc_trait_t::construct(m_alloc, data() + m_size++, std::forward<Args>(args)...);
        }

        auto insert(iterator pos, const_reference val) -> iterator {
            auto p = std::distance(begin(), pos);
            auto n = static_cast<difference_type>(size()) - p;
            reserve(size() + 1);
            ++m_size;
            std::move(rbegin() + 1, rbegin() + n + 1, rbegin());
            alloc_trait_t::construct(m_alloc, data() + p, val);
            return begin() + p;
        }

        auto insert(iterator pos, value_type&& val) -> iterator {
            auto p = std::distance(begin(), pos);
            auto n = static_cast<difference_type>(size()) - p;
            reserve(size() + 1);
            ++m_size;
            std::move(rbegin() + 1, rbegin() + n + 1, rbegin());
            alloc_trait_t::construct(m_alloc, data() + p, val);
            return begin() + p;
        }

        auto insert(iterator pos, std::span<value_type> s) -> iterator {
            auto p = std::distance(begin(), pos);
            auto n = static_cast<difference_type>(size()) - p;
            auto ssize = static_cast<difference_type>(s.size());
            reserve(size() + s.size());
            m_size += s.size();
            std::move(rbegin() + ssize, rbegin() + n + ssize, rbegin());
            auto it = begin() + p;

            for(auto i = 0ul; i < s.size(); ++i, ++p) {
                alloc_trait_t::construct(m_alloc, data() + p, std::move(s[i]));
            }

            return it;
        }

        auto back() -> reference {
            assert(!empty());
            return data()[m_size - 1];
        }

        auto back() const -> const_reference {
            auto self = const_cast<SmallVec*>(this);
            return self->back();
        }


        auto front() -> reference {
            assert(!empty());
            return data()[0];
        }

        auto front() const -> const_reference {
            auto self = const_cast<SmallVec*>(this);
            return self->front();
        }

        auto data() noexcept -> pointer {
            if (is_small()) return reinterpret_cast<pointer>(m_data.static_);
            return reinterpret_cast<pointer>(m_data.dyn);
        }

        auto data() const noexcept -> const_pointer {
            auto& self = *const_cast<SmallVec*>(this);
            return self.data();
        }

        auto begin() noexcept -> iterator { return data(); }
        auto end() noexcept -> iterator { return data() + size(); }

        auto begin() const noexcept -> const_iterator { return data(); }
        auto end() const noexcept -> const_iterator { return data() + size(); }

        auto rbegin() noexcept -> reverse_iterator {
            return std::reverse_iterator(end());
        }
        auto rend() noexcept -> reverse_iterator {
            return std::reverse_iterator(begin());
        }

        auto rbegin() const noexcept -> const_reverse_iterator {
            return std::reverse_iterator(end());
        }
        auto rend() const noexcept -> const_reverse_iterator {
            return std::reverse_iterator(begin());
        }

        constexpr auto is_small() const noexcept -> bool {
            return capacity() <= Cap;
        }

        operator std::span<T>() const noexcept {
            auto self = const_cast<SmallVec*>(this);
            return { self->data(), size() };
        }

        auto operator[](size_type k) noexcept -> reference {
            assert(k < size());
            return data()[k];
        }
        auto operator[](size_type k) const noexcept -> const_reference {
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
            if (n <= capacity()) return;

            if (is_small()) {
                if (n > Cap) {
                    auto ns = capacity();
                    while (ns < n) ns *= growth_factor;

                    auto ptr = m_alloc.allocate(ns);
                    std::move(begin(), end(), ptr);
                    m_data.dyn = reinterpret_cast<TypeWrapper*>(ptr);
                    m_capacity = ns;
                }
            } else {
                auto ns = capacity();
                while (ns < n) ns *= growth_factor;

                auto ptr = m_alloc.allocate(ns);
                std::move(begin(), end(), ptr);
                auto old_ptr = reinterpret_cast<pointer>(m_data.dyn);
                m_alloc.deallocate(old_ptr, capacity());
                m_capacity = ns;
                m_data.dyn = reinterpret_cast<TypeWrapper*>(ptr);
            }
        }

        auto resize(size_type n) {
            if (n == size()) return;

            reserve(n);
            m_size = n;

            for (auto it = begin() + n; it != end(); ++it) {
                it->~T();
            }
        }

        auto resize(size_type n, T const& def) {
            auto old_size = size();
            resize(n);
            if (old_size < n) {
                for (auto it = begin() + old_size; it != end(); ++it) {
                    *it = def;
                }
            }
        }

        auto clear() noexcept(std::is_nothrow_destructible_v<T>) {
            for (auto& tmp: *this) {
                tmp.~T();
            }
            m_size = 0;
        }

        auto erase(iterator pos) -> void {
            assert(pos >= begin() && pos < end());
            std::move(pos + 1, end(), pos);
            m_size -= 1;
        }

        auto erase(iterator first, iterator last) -> void {
            auto diff = static_cast<size_type>(last - first);
            if (diff >= size()) {
                clear();
                return;
            }
            std::move(last, end(), first);
            m_size -= diff;
        }

        auto operator==(SmallVec const& other) const noexcept -> bool {
            if (size() != other.size()) return false;
            return std::equal(begin(), end(), other.begin());
        }

        friend auto swap(SmallVec& lhs, SmallVec& rhs) noexcept -> void {
            using std::swap;
            swap(lhs.m_alloc, rhs.m_alloc);
            swap(lhs.m_data, rhs.m_data);
            swap(lhs.m_size, rhs.m_size);
            swap(lhs.m_capacity, rhs.m_capacity);
        }
    private:
        auto get(size_type k) noexcept -> TypeWrapper* {
            assert(k < size());
            if (is_small()) return m_data.static_ + k;
            return m_data.dyn + k;
        }

        auto get(size_type k) const noexcept -> TypeWrapper const* {
            assert(k < size());
            if (is_small()) return m_data.static_ + k;
            return m_data.dyn + k;
        }
    private:
        allocator_t m_alloc{};
        Wrapper m_data;
        size_type m_size{};
        size_type m_capacity{Cap};
    };

    template <typename T, typename A>
    struct SmallVec<T, 0, A> {
        using base_type = std::pmr::vector<T>;
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

        auto insert(iterator pos, const_reference val) -> iterator {
            return m_data.insert(pos, val);
        }

        auto insert(iterator pos, value_type&& val) -> iterator {
            return m_data.insert(pos, std::move(val));
        }

        auto insert(iterator pos, std::span<value_type> s) -> iterator {
            return m_data.insert(pos, s.begin(), s.end());
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
