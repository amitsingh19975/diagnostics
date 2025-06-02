#ifndef DARK_DIAGNOSTICS_SPAN_HPP
#define DARK_DIAGNOSTICS_SPAN_HPP

#include "core/small_vector.hpp"
#include <algorithm>
#include <format>
#include <cassert>
#include <cstddef>
#include <optional>
#include <ostream>
#include <type_traits>

namespace dark {

    enum class SpanKind {
        Absolute,
        MarkerRel,
        LocRel
    };

    /*
        Representing open-ended range. [a, b)
    */
    template <SpanKind Kind>
    class BasicSpan {
        static constexpr auto is_signed = (Kind == SpanKind::MarkerRel);
        static constexpr auto is_absolute_kind = (Kind == SpanKind::Absolute);
    public:
        using base_type = std::conditional_t<is_signed, int, unsigned>;
        using size_type = unsigned;

        constexpr BasicSpan() noexcept = default;
        constexpr BasicSpan(BasicSpan const&) noexcept = default;
        constexpr BasicSpan& operator=(BasicSpan const&) noexcept = default;
        constexpr BasicSpan(BasicSpan &&) noexcept = default;
        constexpr BasicSpan& operator=(BasicSpan &&) noexcept = default;
        constexpr ~BasicSpan() noexcept = default;

        explicit constexpr BasicSpan(base_type start, base_type end) noexcept
            : m_start(start)
        {
            auto const size = end - start;
            if (size < 0) {
                m_start = 0;
                m_size = 0;
            } else {
                m_size = static_cast<unsigned>(size);
            }
        }

        explicit constexpr BasicSpan(base_type start) noexcept
            : m_start(start)
            , m_size(1)
        {}

        constexpr base_type start() const noexcept { return m_start; }
        constexpr base_type end() const noexcept { return m_start + static_cast<base_type>(size()); }
        constexpr size_type size() const noexcept { return m_size; }

        static constexpr auto from_size(base_type start, size_type size) noexcept -> BasicSpan {
            auto temp = BasicSpan();
            temp.m_start = start;
            temp.m_size = size;
            return temp;
        }

        static constexpr auto from_usize(base_type start, std::size_t size) noexcept -> BasicSpan {
            return from_size(start, static_cast<unsigned>(size));
        }

        constexpr BasicSpan<SpanKind::Absolute> resolve(unsigned base) const noexcept requires (Kind != SpanKind::Absolute) {
            if constexpr (is_signed) {
                auto n_base = static_cast<std::ptrdiff_t>(base);
                auto n_start = static_cast<std::ptrdiff_t>(start()) + n_base;
                auto size = this->size();
                if (n_start < 0) {
                    n_start = 0;
                    size = 0;
                }
                return BasicSpan<SpanKind::Absolute>::from_size(static_cast<unsigned>(n_start), size);
            } else {
                return BasicSpan<SpanKind::Absolute>::from_size(m_start + base, size());
            }
        }

        constexpr auto shift(std::ptrdiff_t offset) const noexcept -> BasicSpan {
            auto res = *this;
            if constexpr (is_signed) {
                res.m_start += offset;
            } else {
                //               |
                //               |
                //               |
                //         [-----|----------------)
                //               |    |
                //               |    v
                //               |[---------------)
                //               |
                auto temp = static_cast<std::ptrdiff_t>(res.m_start) + offset;
                if (temp < 0) {
                    auto diff = std::max(static_cast<unsigned>(-offset), res.m_start) - res.m_start;
                    res.m_start = 0;
                    res.m_size = res.m_size - std::min(res.m_size, diff);
                } else {
                    res.m_start = static_cast<unsigned>(temp);
                }
            }
            return res;
        }

        constexpr auto is_absolute() const noexcept -> bool {
            return Kind == SpanKind::Absolute;
        }

        constexpr auto is_marker_relative() const noexcept -> bool {
            return Kind == SpanKind::MarkerRel;
        }
        
        constexpr auto is_loc_relative() const noexcept -> bool {
            return Kind == SpanKind::LocRel;
        }

        constexpr auto clip_end(base_type end) const noexcept -> BasicSpan requires (is_absolute_kind) {
            return BasicSpan(start(), std::min(end, this->end()));
        }

        constexpr auto clip_start(base_type start) const noexcept -> BasicSpan requires (is_absolute_kind) {
            return BasicSpan(std::max(start, this->start()), end());
        }

        constexpr auto intersects(BasicSpan other, bool inclusive = false) const noexcept -> bool requires (is_absolute_kind) {
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
            //              [----------)
            //
            // Case 4: No Intersection
            // [------------)
            //                  [----------)
            //

            auto lhs = *this;
            auto rhs = other;
            if (m_start > other.start()) {
                swap(lhs, rhs);
            }

            return inclusive ? (rhs.start() <= lhs.end()) : (rhs.start() < lhs.end());
        }

        constexpr auto get_intersections(BasicSpan const& other) const noexcept -> core::SmallVec<BasicSpan, 3> requires (is_absolute_kind) {
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

            auto lhs = *this; // comes first
            auto rhs = other; // comes last
            if (m_start > other.start()) {
                swap(lhs, rhs);
            }

            if (!lhs.intersects(rhs)) return { lhs, rhs };

            if (lhs == rhs) return { lhs };

            // Completely inside the range.
            if (rhs.end() <= lhs.end()) {
                return {
                    BasicSpan(lhs.start(), rhs.start()),
                    BasicSpan(rhs.start(), rhs.end()),
                    BasicSpan(rhs.end(), lhs.end()),
                };
            }

            return {
                BasicSpan(lhs.start(), rhs.start()),
                BasicSpan(rhs.start(), lhs.end()),
                BasicSpan(lhs.end(), rhs.end())
            };
        }

        constexpr auto force_merge(BasicSpan const& other) const noexcept -> BasicSpan requires (is_absolute_kind) {
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

            auto ss = get_intersections(other);
            auto res = BasicSpan();
            for (auto const& s: ss) {
                res = BasicSpan(std::min(start(), s.start()), std::max(end(), s.end()));
            }
            return res;
        }

        constexpr auto merge(BasicSpan const& other) const noexcept -> std::optional<BasicSpan> requires (is_absolute_kind) {
            if (!intersects(other, true)) return std::nullopt;
            return force_merge(other);
        }

        constexpr auto empty() const noexcept -> bool {
            return size() == 0;
        }

        constexpr auto operator==(BasicSpan const& other) const noexcept -> bool {
            return start() == other.start() && size() == other.size();
        }

        constexpr auto operator!=(BasicSpan const& other) const noexcept -> bool {
            return !(*this == other);
        }

        friend auto swap(BasicSpan& lhs, BasicSpan& rhs) noexcept -> void {
            using std::swap;
            swap(lhs.m_size, rhs.m_size);
            swap(lhs.m_start, rhs.m_start);
        }

        friend auto operator<<(std::ostream& os, BasicSpan const& s) -> std::ostream& {
            return os << std::format("Span(start: {}, end: {}, size: {})", s.start(), s.end(), s.size());
        }

        constexpr auto is_between(BasicSpan parent) const noexcept requires (is_absolute_kind) {
            return parent.start() >= start() && end() <= parent.end();
        }

    private:
        base_type m_start{};
        size_type m_size{};
    };

    using Span = BasicSpan<SpanKind::Absolute>;
    using MarkerRelSpan = BasicSpan<SpanKind::MarkerRel>;
    using LocRelSpan = BasicSpan<SpanKind::LocRel>;

    namespace detail {

        template <typename T>
        struct is_basic_span_helper: std::false_type {};

        template <SpanKind K>
        struct is_basic_span_helper<BasicSpan<K>>: std::true_type {};

        template <typename T>
        concept is_basic_span = is_basic_span_helper<std::decay_t<std::remove_cvref_t<T>>>::value;

    } // namespace detail

} // namespace dark

#endif // DARK_DIAGNOSTICS_SPAN_HPP
