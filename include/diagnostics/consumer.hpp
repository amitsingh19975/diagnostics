#ifndef DARK_DIAGNOSTICS_CONSUMER_HPP
#define DARK_DIAGNOSTICS_CONSUMER_HPP

#include "basic.hpp"
#include "diagnostics/core/small_vector.hpp"
#include "diagnostics/printer.hpp"
#include <algorithm>
#include <cassert>

namespace dark {

    template <typename DiagnosticKindType>
    class DiagnosticConsumer {
    public:
        virtual ~DiagnosticConsumer() = default;
        virtual auto consume(BasicDiagnostic<DiagnosticKindType>&& diagnostic) -> void = 0;
        virtual auto flush() -> void {}
    };

    template <typename DiagnosticKindType = EmptyDiagnosticKind>
    class BasicStreamDiagnosticConsumer: public DiagnosticConsumer<DiagnosticKindType> {
    public:
        explicit constexpr BasicStreamDiagnosticConsumer(Stream& os) noexcept
            : m_stream(&os)
        {}
        BasicStreamDiagnosticConsumer(BasicStreamDiagnosticConsumer const&) = default;
        BasicStreamDiagnosticConsumer(BasicStreamDiagnosticConsumer&&) = default;
        BasicStreamDiagnosticConsumer& operator=(BasicStreamDiagnosticConsumer const&) = default;
        BasicStreamDiagnosticConsumer& operator=(BasicStreamDiagnosticConsumer&&) = default;
        ~BasicStreamDiagnosticConsumer() override = default;

        auto consume(BasicDiagnostic<DiagnosticKindType>&& diagnostic) -> void override {
            auto screen = internal::draw_diagnostics(std::move(diagnostic));
            *m_stream << screen << '\n';
        }

        auto flush() -> void override { m_stream->flush(); }

        constexpr auto reset() noexcept -> void {
            m_has_printed = false;
        }

    private:
        Stream* m_stream;
        bool m_has_printed{ false };
    };

    template <typename DiagnosticKindType = EmptyDiagnosticKind>
    static inline auto ConsoleDiagnosticConsumer() noexcept -> DiagnosticConsumer<DiagnosticKindType>& {
        static auto* consumer = new BasicStreamDiagnosticConsumer<DiagnosticKindType>(err());
        return *consumer;
    }

    template <typename DiagnosticKindType = EmptyDiagnosticKind>
    class BasicErrorTrackingDiagnosticConsumer: public DiagnosticConsumer<DiagnosticKindType> {
    public:
        explicit constexpr BasicErrorTrackingDiagnosticConsumer(DiagnosticConsumer<DiagnosticKindType>* consumer) noexcept
            : m_consumer(consumer)
        {}
        constexpr BasicErrorTrackingDiagnosticConsumer(BasicErrorTrackingDiagnosticConsumer const&) noexcept = default;
        constexpr BasicErrorTrackingDiagnosticConsumer(BasicErrorTrackingDiagnosticConsumer&&) noexcept = default;
        constexpr BasicErrorTrackingDiagnosticConsumer& operator=(BasicErrorTrackingDiagnosticConsumer const&) noexcept = default;
        constexpr BasicErrorTrackingDiagnosticConsumer& operator=(BasicErrorTrackingDiagnosticConsumer&&) noexcept = default;
        constexpr ~BasicErrorTrackingDiagnosticConsumer() override = default;

        auto consume(BasicDiagnostic<DiagnosticKindType>&& diagnostic) -> void override {
            m_seen_error |= (diagnostic.level == DiagnosticLevel::Error);
            m_consumer->consume(std::move(diagnostic));
        }

        auto flush() -> void override { m_consumer->flush(); }

        [[nodiscard]] constexpr auto seen_error() const noexcept -> bool {
            return m_seen_error;
        }

        constexpr auto reset() noexcept -> void {
            m_seen_error = false;
        }
    private:
        DiagnosticConsumer<DiagnosticKindType>* m_consumer;
        bool m_seen_error{ false };
    };

    template <typename DiagnosticKindType>
    BasicErrorTrackingDiagnosticConsumer(DiagnosticConsumer<DiagnosticKindType>*) -> BasicErrorTrackingDiagnosticConsumer<DiagnosticKindType>;

    template <typename DiagnosticKindType = EmptyDiagnosticKind>
    class BasicSortingDiagnosticConsumer: public DiagnosticConsumer<DiagnosticKindType> {
    public:
        explicit constexpr BasicSortingDiagnosticConsumer(DiagnosticConsumer<DiagnosticKindType>* consumer) noexcept
            : m_consumer(consumer)
        {}

        BasicSortingDiagnosticConsumer(BasicSortingDiagnosticConsumer const&) = default;
        BasicSortingDiagnosticConsumer(BasicSortingDiagnosticConsumer&&) = default;
        BasicSortingDiagnosticConsumer& operator=(BasicSortingDiagnosticConsumer const&) = default;
        BasicSortingDiagnosticConsumer& operator=(BasicSortingDiagnosticConsumer&&) = default;
        ~BasicSortingDiagnosticConsumer() override {
            assert(m_diagnostics.empty() && "Diagnostics is not flushed");
        }

        auto consume(BasicDiagnostic<DiagnosticKindType>&& diagnostic) -> void override {
            m_diagnostics.push_back(std::move(diagnostic));
        }

        auto flush() -> void override {
            // 1. Sort the diagnostics
            std::stable_sort(m_diagnostics.begin(), m_diagnostics.end(), [](auto const& l, auto const& r) {
                return l.location < r.location;
            });

            // 2. Flush the diagnostics
            for (auto& diag: m_diagnostics) {
                m_consumer->consume(std::move(diag));
            }

            m_consumer->flush();
        }

    private:
        DiagnosticConsumer<DiagnosticKindType>* m_consumer;
        core::SmallVec<BasicDiagnostic<DiagnosticKindType>, 0> m_diagnostics{};
    };

    template <typename DiagnosticKindType>
    BasicSortingDiagnosticConsumer(DiagnosticConsumer<DiagnosticKindType>*) -> BasicSortingDiagnosticConsumer<DiagnosticKindType>;

} // namespace dark

#endif // DARK_DIAGNOSTICS_CONSUMER_HPP
