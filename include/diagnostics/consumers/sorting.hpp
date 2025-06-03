#ifndef AMT_DARK_DIAGNOSTICS_CONSUMERS_SORTING_HPP
#define AMT_DARK_DIAGNOSTICS_CONSUMERS_SORTING_HPP

#include "base.hpp"
#include "../core/small_vec.hpp"
#include <cassert>

namespace dark {
    struct SortingDiagnosticConsumer: DiagnosticConsumer {
        explicit constexpr SortingDiagnosticConsumer(DiagnosticConsumer* consumer) noexcept
            : m_consumer(consumer)
        {}
        constexpr SortingDiagnosticConsumer(SortingDiagnosticConsumer const&) noexcept = default;
        constexpr SortingDiagnosticConsumer(SortingDiagnosticConsumer &&) noexcept = default;
        constexpr SortingDiagnosticConsumer& operator=(SortingDiagnosticConsumer const&) noexcept = default;
        constexpr SortingDiagnosticConsumer& operator=(SortingDiagnosticConsumer &&) noexcept = default;

        #ifdef NDEBUG
        ~SortingDiagnosticConsumer() noexcept override = default;
        #else
        ~SortingDiagnosticConsumer() noexcept override {
            assert(m_diagnostics.empty() && "Diagnostics are not flushed");
        }
        #endif

        auto consume(Diagnostic&& d) -> void override {
            m_diagnostics.emplace_back(std::move(d));
        }

        auto flush() -> void override {
            std::stable_sort(m_diagnostics.begin(), m_diagnostics.end(), [](Diagnostic const& l, Diagnostic const& r) {
                return l.location.line_info() < r.location.line_info();
            });

            for (auto& diag: m_diagnostics) m_consumer->consume(std::move(diag));
            m_diagnostics.clear();
            m_consumer->flush();
        }
    private:
        DiagnosticConsumer* m_consumer;
        core::SmallVec<Diagnostic, 0> m_diagnostics{};
    };
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_CONSUMERS_SORTING_HPP
