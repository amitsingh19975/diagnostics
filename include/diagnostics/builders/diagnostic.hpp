#ifndef AMT_DARK_DIAGNOSTICS_BUILDERS_DIAGNOSTIC_HPP
#define AMT_DARK_DIAGNOSTICS_BUILDERS_DIAGNOSTIC_HPP

#include "../forward.hpp"
#include "../basic.hpp"
#include "../core/format.hpp"
#include <cassert>

namespace dark::builder {
    template <typename LocT>
    struct DiagnosticBuilder {
        using emitter_t = DiagnosticEmitter<LocT>;

        DiagnosticBuilder(DiagnosticBuilder const&) noexcept = delete;
        constexpr DiagnosticBuilder(DiagnosticBuilder &&) noexcept = default;
        DiagnosticBuilder& operator=(DiagnosticBuilder const&) noexcept = delete;
        constexpr DiagnosticBuilder& operator=(DiagnosticBuilder &&) noexcept = default;
        constexpr ~DiagnosticBuilder() = default;

        auto emit() -> void {
            for (auto& annot: m_emitter->m_annotations) {
                annot(*this);
            }
            m_emitter->m_consumer->consume(std::move(m_diagnostic));
        }
    private:
        friend struct DiagnosticEmitter<LocT>;

        template <core::IsFormattable... Args>
        DiagnosticBuilder(
            emitter_t* emitter,
            LocT loc,
            DiagnosticLevel level,
            internal::DiagnosticBase<Args...> const& base,
            core::BasicFormatter formatter
        )
            : m_emitter(emitter)
        {
            assert((level != DiagnosticLevel::Note) && "Note messages cannot be a base diagnostic level");
            add_message(std::move(loc), level, base, std::move(formatter));
        }

        template <core::IsFormattable... Args>
        auto add_message(
            LocT loc,
            DiagnosticLevel level,
            internal::DiagnosticBase<Args...> const& base,
            core::BasicFormatter formatter
        ) -> void {
            add_message(
                level,
                m_emitter->m_converter->convert_loc(loc, *this),
                base,
                std::move(formatter)
            );
        }

        template <core::IsFormattable... Args>
        auto add_message(
            DiagnosticLevel level,
            DiagnosticLocation location,
            internal::DiagnosticBase<Args...> const& base,
            core::BasicFormatter formatter
        ) -> void {
            m_diagnostic.level = level;
            m_diagnostic.kind = base.kind;
            m_diagnostic.location = std::move(location);
            m_diagnostic.message = std::move(formatter);
            m_diagnostic.annotations.clear();
        }
    private:
        emitter_t* m_emitter;
        Diagnostic m_diagnostic;
    };

} // namespace dark::builder

#endif // AMT_DARK_DIAGNOSTICS_BUILDERS_DIAGNOSTIC_HPP
