#ifndef DARK_DIAGNOSTICS_CORE_SMALL_VECTOR_HPP
#define DARK_DIAGNOSTICS_CORE_SMALL_VECTOR_HPP

#include <algorithm>
#include <cassert>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <variant>
#include <vector>
#include <array>
#include <type_traits>

namespace dark::core {

    template<typename T, std::size_t N = 7>
    class SmallVec {
        static constexpr bool is_dynamic_size = (N == 0);
        using base_type = std::conditional_t<
            is_dynamic_size,
            std::vector<T>,
            std::variant<std::array<T, N>, std::vector<T>>
        >;
    public:
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;
        using reference = T&;
        using const_reference = T const&;
        using iterator = T*;
        using const_iterator = T const*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        constexpr SmallVec() noexcept requires (!is_dynamic_size)
            : m_data(std::array<T, N>())
        {}
        constexpr SmallVec() requires is_dynamic_size = default;
        SmallVec(SmallVec const&) = default;
        SmallVec& operator=(SmallVec const&) = default;
        constexpr SmallVec(SmallVec&&) noexcept = default;
        constexpr SmallVec& operator=(SmallVec&&) noexcept = default;
        constexpr ~SmallVec() = default;

        SmallVec(std::initializer_list<T> li) requires is_dynamic_size
            : m_data(li)
        {}

        SmallVec(std::initializer_list<T> li) requires (!is_dynamic_size) {
            if (li.size() > N) {
                m_data = std::vector(li);
            } else {
                auto temp = std::array<T, N>{};
                std::copy(li.begin(), li.end(), temp.begin());
                m_data = std::move(temp);
                m_size = li.size();
            }
        }

        template <typename U, std::size_t M>
        auto extend(SmallVec<U, M> v) -> void {
            if constexpr (is_dynamic_size) {
                m_data.reserve(m_data.size() + std::distance(v.begin(),v.end()));
                m_data.insert(m_data.end(),v.begin(),v.end());
            } else {
                if (m_data.index() == 1) {
                    auto& data = std::get<1>(m_data);
                    data.reserve(data.size() + static_cast<std::size_t>(std::distance(v.begin(),v.end())));
                    data.insert(data.end(),v.begin(),v.end());
                } else {
                    auto& data = std::get<0>(m_data);
                    if (m_size + v.size() > N) {
                        auto temp = std::vector<T>(data.size() + v.size());
                        std::move(data.begin(), data.end(), temp.begin());
                        std::move(v.begin(), v.end(), temp.begin() + static_cast<std::ptrdiff_t>(data.size()));
                        m_data = std::move(temp);
                    } else {
                        auto size = v.size();
                        std::move(v.begin(), v.end(), data.begin() + static_cast<std::ptrdiff_t>(m_size));
                        m_size += size;
                    }
                }
            }
        }

        auto clear() -> void {
            if constexpr (is_dynamic_size) {
                m_data.clear();
            } else {
                if (m_data.index() == 1) {
                    auto& data = std::get<1>(m_data);
                    data.clear();
                } else {
                    auto& data = std::get<0>(m_data);
                    for (auto& el: data) {
                        el.~T();
                    }
                    m_size = 0;
                }
            }
        }

        auto reserve(size_type size) {
            if constexpr (is_dynamic_size) {
                m_data.reserve(size);
            } else {
                if (m_data.index() == 0) {
                    auto& data = std::get<0>(m_data);
                    if (m_size > size) {
                        auto temp = std::vector<T>{};
                        temp.reserve(size);
                        for (auto& el : data) temp.push_back(std::move(el));
                        m_data = std::move(temp);
                    }
                } else {
                    std::get<1>(m_data).reserve(size);
                }
            }
        }

        constexpr auto empty() const noexcept -> bool {
            if constexpr (is_dynamic_size) return m_data.empty();
            else {
                if (m_data.index() == 0) return m_size == 0;
                else return std::get<1>(m_data).empty();
            }
        }

        constexpr auto size() const noexcept -> size_type {
            if constexpr (is_dynamic_size) return m_data.size();
            else {
                if (m_data.index() == 0) return m_size;
                else return std::get<1>(m_data).size();
            }
        }

        constexpr auto begin() noexcept -> iterator {
            if constexpr (is_dynamic_size) return m_data.data();
            else return std::visit([](auto& v) { return v.data(); }, m_data);
        }

        constexpr auto end() noexcept -> iterator {
            return begin() + size();
        }

        constexpr auto begin() const noexcept -> const_iterator {
            if constexpr (is_dynamic_size) return m_data.data();
            else return std::visit([](auto& v) { return v.data(); }, m_data);
        }

        constexpr auto end() const noexcept -> const_iterator {
            return begin() + size();
        }

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

        
        auto push_back(T&& val) -> void {
            push_back_helper(std::move(val));
        }
       
        auto push_back(T const& val) -> void {
            push_back_helper(val);
        }

        template <typename... Args>
            requires std::constructible_from<T, Args...>
        auto emplace_back(Args&&... args) {
            if constexpr (is_dynamic_size) {
                m_data.emplace_back(std::forward<Args>(args)...);
            } else {
                if (m_data.index() == 0) {
                    auto& v = std::get<0>(m_data);
                    if (m_size >= N) {
                        auto temp = std::vector<T>(v.size());
                        std::move(v.begin(), v.end(), temp.begin());
                        temp.emplace_back(std::forward<Args>(args)...);
                        m_data = temp;
                    } else {
                        v[m_size++] = T(std::forward<Args>(args)...);
                    }
                } else {
                    std::get<1>(m_data).emplace_back(std::forward<Args>(args)...);
                }
            }
        }

        auto pop_back() -> T {
            assert(!empty() && "SmallVec: cannot pop an empty vector.");
            if constexpr (is_dynamic_size) {
                auto res = std::move(m_data[m_data.size() - 1]);
                m_data.pop_back();
                return res;
            } else {
                if (m_data.index() == 0) {
                    auto& v = std::get<0>(m_data);
                    auto last = std::move(v[(m_size--) - 1]);
                    return last;
                } else {
                    auto& v = std::get<1>(m_data);
                    auto res = std::move(v[v.size() - 1]);
                    v.pop_back();
                    return res;
                }
            }
        }

        auto erase(iterator start_it, iterator end_it) {
            auto start = start_it - begin();
            auto end = end_it - begin();

            if constexpr (is_dynamic_size) {
                m_data.erase(m_data.begin() + start, m_data.begin() + end);
            } else {
                if (m_data.index() == 0) {
                    auto& a = std::get<0>(m_data);
                    auto it = start_it;
                    while(it < end_it) {
                        (*it).~T();
                        ++it;
                    }

                    auto start_index = static_cast<size_type>(start);
                    auto end_index = static_cast<size_type>(end);

                    for (auto e = 0zu; e + end_index < m_size; ++e) {
                        a[start_index + e] = std::move(a[end_index + e]);
                    }

                    m_size = std::max(m_size - (end_index - start_index), 0zu);
                } else {
                    auto& v = std::get<1>(m_data);
                    v.erase(v.begin() + start, v.begin() + end);
                }
            }
        }

        auto remove(size_type pos, size_type size = 1) {
            if (pos >= this->size()) return;

            auto start = begin() + static_cast<difference_type>(pos);
            auto end_ = start + static_cast<difference_type>(size);
            if (end_ >= end()) end_ = end();
            erase(start, end_);
        }

        template <typename U>
        auto insert(size_type pos, U&& val) {
            if constexpr (is_dynamic_size) {
                std::vector<T>& data = m_data;
                data.insert(data.begin() + static_cast<std::ptrdiff_t>(pos), std::move(val));
            } else {
                if (m_data.index() == 0) {
                    auto& a = std::get<0>(m_data);
                    m_size = m_size + 1;
                    if (m_size > N) {
                        std::vector<T> data;
                        data.reserve(m_size);
                        auto i = 0zu;
                        for (; i < pos; ++i) {
                            data.push_back(std::move(a[i]));
                        }
                        data.push_back(std::move(val));
                        for (; i < m_size - 1; ++i) {
                            data.push_back(std::move(a[i]));
                        }
                        m_data = std::move(data);
                    } else {
                        if (pos == m_size - 1) {
                            a[m_size - 1] = std::move(val);
                        } else {
                            for (auto i = m_size - 1; i > pos; --i) {
                                a[i] = std::move(a[i - 1]);
                            }
                            a[pos + 1] = std::move(a[pos]);
                            a[pos] = std::move(val);
                        }
                    }
                } else {
                    auto& data = std::get<1>(m_data);
                    data.insert(data.begin() + static_cast<std::ptrdiff_t>(pos), std::move(val));
                }
            }
        }

        auto resize(size_type size, T def = {}) {
            auto last_size = this->size();
            reserve(size);
            for (auto i = last_size; i < size; ++i) {
                push_back(def);
            } 
        }

        constexpr auto is_dynamic() const noexcept -> bool {
            if constexpr (is_dynamic_size) return true;
            else return m_data.index() == 1;
        }

        constexpr auto is_static() const noexcept -> bool {
            return !is_dynamic();
        }

        constexpr auto operator[](size_type k) noexcept -> reference {
            if constexpr (is_dynamic_size) return m_data[k];
            else return std::visit([k](auto& v) -> reference { return v[k]; }, m_data);
        }

        constexpr auto operator[](size_type k) const noexcept -> const_reference {
            if constexpr (is_dynamic_size) return m_data[k];
            else return std::visit([k] (auto const& v) -> const_reference { return v[k]; }, m_data);
        }

        constexpr auto at(size_type k) -> reference {
            if constexpr (is_dynamic_size) return m_data.at(k);
            else return std::visit([k](auto& v) -> reference { return v.at(k); }, m_data);
        }

        constexpr auto at(size_type k) const -> const_reference {
            if constexpr (is_dynamic_size) return m_data.at(k);
            else return std::visit([k](auto const & v) -> const_reference { return v.at(k); }, m_data);
        }

        constexpr auto back() const noexcept -> const_reference {
            return this->operator[](size() - 1);
        }

        constexpr auto back() noexcept -> reference {
            return this->operator[](size() - 1);
        }

    private:
        template <typename U>
        auto push_back_helper(U&& val) -> void {
            if constexpr (is_dynamic_size) {
                m_data.push_back(std::forward<U>(val));
            } else {
                if (m_data.index() == 0) {
                    auto& v = std::get<0>(m_data);
                    if (m_size >= N) {
                        auto temp = std::vector<T>(v.size());
                        std::move(v.begin(), v.end(), temp.begin());
                        temp.push_back(std::forward<U>(val));
                        m_data = temp;
                    } else {
                        v[m_size++] = std::move(val);
                    }
                } else {
                    std::get<1>(m_data).push_back(std::forward<U>(val));
                }
            }
        }

    private:
        base_type m_data;
        size_type m_size{0};
    };

} // namespace dark::core

#endif // DARK_DIAGNOSTICS_CORE_SMALL_VECTOR_HPP
