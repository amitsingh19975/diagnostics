#ifndef AMT_DARK_DIAGNOSTICS_CONSUMERS_ERROR_TRACKING_HPP
#define AMT_DARK_DIAGNOSTICS_CONSUMERS_ERROR_TRACKING_HPP

#include "base.hpp"
#include <cassert>

namespace dark {
    struct ErrorTrackingDiagnosticConsumer: DiagnosticConsumer {
        explicit constexpr ErrorTrackingDiagnosticConsumer(DiagnosticConsumer* consumer) noexcept
            : m_consumer(consumer)
        {}
        constexpr ErrorTrackingDiagnosticConsumer(ErrorTrackingDiagnosticConsumer const&) noexcept = default;
        constexpr ErrorTrackingDiagnosticConsumer(ErrorTrackingDiagnosticConsumer &&) noexcept = default;
        constexpr ErrorTrackingDiagnosticConsumer& operator=(ErrorTrackingDiagnosticConsumer const&) noexcept = default;
        constexpr ErrorTrackingDiagnosticConsumer& operator=(ErrorTrackingDiagnosticConsumer &&) noexcept = default;
        ~ErrorTrackingDiagnosticConsumer() noexcept override = default;

        auto consume(Diagnostic&& d) -> void override {
            m_seen_error |= d.level == DiagnosticLevel::Error;
            m_consumer->consume(std::move(d));
        }

        auto flush() -> void override { m_consumer->flush(); }

        constexpr auto seen_error() const noexcept -> bool { return m_seen_error; }

        constexpr auto reset() noexcept -> void {
            m_seen_error = false;
        }
    private:
        DiagnosticConsumer* m_consumer;
        bool m_seen_error{false};
    };
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_CONSUMERS_ERROR_TRACKING_HPP
