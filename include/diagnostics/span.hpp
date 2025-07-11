#ifndef AMT_DARK_DIAGNOSTICS_SPAN_HPP
#define AMT_DARK_DIAGNOSTICS_SPAN_HPP

#include "core/small_vec.hpp"
#include "core/config.hpp"
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <format>
#include <ostream>
#include <type_traits>

namespace dark {

    struct Span {
        using size_type = dsize_t;
        constexpr Span() noexcept = default;
        constexpr Span(Span const&) noexcept = default;
        constexpr Span(Span &&) noexcept = default;
        constexpr Span& operator=(Span const&) noexcept = default;
        constexpr Span& operator=(Span &&) noexcept = default;
        constexpr ~Span() noexcept = default;

        constexpr Span(size_type start, size_type end) noexcept
            : m_start(start)
            , m_size(std::max(start, end) - start)
        {}

        constexpr Span(size_type start) noexcept
            : m_start(start)
            , m_size(1)
        {}

        constexpr auto start() const noexcept -> size_type { return m_start; }
        constexpr auto size() const noexcept -> size_type { return m_size; }
        constexpr auto end() const noexcept -> size_type { return start() + size(); }
        constexpr auto empty() const noexcept -> bool { return size() == 0; }

        static constexpr auto from_size(size_type start, size_type size) noexcept -> Span {
            return Span(start, start + size);
        }

        constexpr auto shift(std::ptrdiff_t offset) const noexcept -> Span {
            auto s = static_cast<std::ptrdiff_t>(start()) + offset;
            auto e = std::max<std::ptrdiff_t>(s + size(), 0);
            return Span(
                static_cast<size_type>(std::max<std::ptrdiff_t>(s, 0)),
                static_cast<size_type>(e)
            );
        }

        template <std::integral T>
        constexpr auto operator+(T offset) const noexcept -> Span {
            return shift(static_cast<std::ptrdiff_t>(offset));
        }

        template <std::integral T>
        constexpr auto operator-(T offset) const noexcept -> Span {
            return shift(-static_cast<std::ptrdiff_t>(offset));
        }

        constexpr auto clip_start(size_type start) const noexcept -> Span {
            return Span(
                std::max(start, this->start()),
                end()
            );
        }

        constexpr auto clip_end(size_type end) const noexcept -> Span {
            return Span(
                start(),
                std::max(end, this->end())
            );
        }

        constexpr auto intersects(Span other, bool inclusice = false) const noexcept -> bool {
            if (other.empty() || empty()) return false;
            // Case 1: Intersection
            //          [------------)
            // [----------)
            //
            // Case 2: Intersection
            // [------------)
            //         [----------)
            //
            // Case 3: No Intersection
            // [------------)
            //             - [----------)
            //
            // Case 4: No Intersection
            // [------------)
            //                  [----------)
            //

            auto lhs = *this;
            auto rhs = other;

            if (start() > other.start()) {
                std::swap(lhs, rhs);
            }
            return rhs.start() < lhs.end() + static_cast<dsize_t>(inclusice);
        }

        auto calculate_intersections(Span other) const noexcept -> core::SmallVec<Span, 3> {
            // Case 1: Intersection
            // [-----------)
            //        [---------)
            //       |
            //       V
            // [-----)[----)[-----)
            //
            // Case 2: Intersection
            //        [---------)
            // [-----------)
            //       |
            //       V
            // [-----)[----)[-----)
            //
            // Case 3: No Intersection
            //                 [---------)
            // [-----------)
            //       |
            //       V
            // [-----------)   [---------)
            //

            auto lhs = *this;
            auto rhs = other;

            if (lhs.start() > rhs.start()) {
                std::swap(lhs, rhs);
            }

            if (!lhs.intersects(rhs)) return { lhs, rhs };

            if (lhs == rhs) return { lhs };

            if (rhs.end() <= lhs.end()) {
                auto s1 = Span(lhs.start(), rhs.start());
                auto s2 = Span(rhs.start(), rhs.end());
                auto s3 = Span(rhs.end(), lhs.end());
                return {s1, s2, s3};
            } else {
                auto s1 = Span(lhs.start(), rhs.start());
                auto s2 = Span(rhs.start(), lhs.end());
                auto s3 = Span(lhs.end(), rhs.end());
                return {s1, s2, s3};
            }
        }

        constexpr auto force_merge(Span other) const noexcept -> Span {
            // Case 1: Intersection
            // [-----------)
            //        [---------)
            //       |
            //       V
            // [----------------)
            //
            // Case 2: No Intersection
            // [-----------)
            //                [---------)
            //       |
            //       V
            // [------------------------)

            return Span(
                std::min(start(), other.start()),
                std::max(end(), other.end())
            );
        }

        constexpr auto merge(Span other) const noexcept -> std::optional<Span> {
            if (!intersects(other)) return {};
            return force_merge(other);
        }

        constexpr auto operator==(Span const& other) const noexcept -> bool = default;

        constexpr auto is_between(Span parent, bool inclusive = false) const noexcept -> bool {
            return parent.start() >= start() && (end() + static_cast<dsize_t>(inclusive)) > parent.end();
        }

        constexpr auto is_between(dsize_t el, bool inclusive = false) const noexcept -> bool {
            return el >= start() && el < (end() + static_cast<dsize_t>(inclusive));
        }

        constexpr auto center() const noexcept -> dsize_t { return start() + size() / 2; }

        friend std::ostream& operator<<(std::ostream& os, Span const& s) {
            return os << "Span(start=" << s.start() << ", end=" << s.end() << ", size=" << s.size() << ')';
        }
    private:
        size_type m_start{};
        size_type m_size{};
    };

    namespace detail {
        template <typename S>
        concept IsSpan = std::same_as<std::remove_cvref_t<S>, Span>;
    } // namespace detail

} // namespace dark


template <>
struct std::formatter<dark::Span> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::Span const& s, auto& ctx) const {
        return std::format_to(ctx.out(), "Span(start={}, end={}, size={})", s.start(), s.end(), s.size());
    }
};

template <>
struct std::hash<dark::Span> {
    constexpr auto operator()(dark::Span const& s) const noexcept -> std::size_t {
        auto h0 = std::hash<dark::dsize_t>{}(s.start());
        auto h1 = std::hash<dark::dsize_t>{}(s.size());
        return h0 ^ (h1 << 1);
    }
};


#endif // AMT_DARK_DIAGNOSTICS_SPAN_HPP
