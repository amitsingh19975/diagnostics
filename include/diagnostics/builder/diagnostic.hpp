#ifndef DARK_DIAGNOSTICS_BUILDER_DIAGNOSTIC_HPP
#define DARK_DIAGNOSTICS_BUILDER_DIAGNOSTIC_HPP

#include "diagnostics/basic.hpp"
#include "diagnostics/core/formatter.hpp"
#include "sub_diagnostic.hpp"

namespace dark {
    template <typename LocT, typename DiagnosticKindType>
    class BasicDiagnosticEmitter;
} // namespace dark

namespace dark::internal {
    template <typename LocT, typename DiagnosticKindType>
    class DiagnosticBuilder{
        using emitter_t = BasicDiagnosticEmitter<LocT, DiagnosticKindType>;
    public:
        constexpr DiagnosticBuilder(DiagnosticBuilder const&) noexcept = delete;
        constexpr DiagnosticBuilder& operator=(DiagnosticBuilder const&) noexcept = delete;
        constexpr DiagnosticBuilder(DiagnosticBuilder&&) noexcept = default;
        constexpr DiagnosticBuilder& operator=(DiagnosticBuilder&&) noexcept = default;
        constexpr ~DiagnosticBuilder() = default;

        auto emit() -> void {
            for (auto& annotation: m_emitter->m_annotations) {
                annotation(*this);
            }

            m_emitter->m_consumer->consume(std::move(m_diagnostic));
        }

        template <typename... Args, core::detail::is_valid_formatter_arg... Ts>
            requires ((sizeof...(Args) == sizeof...(Ts)) && (internal::is_format_arg_compatible<Args, Ts>::value && ...))
        auto sub_diagnostic() -> SubDiagnosticBuilder<LocT, DiagnosticKindType> {
            return SubDiagnosticBuilder<LocT, DiagnosticKindType>(*this);
        }

        auto context(DiagnosticContext context) -> DiagnosticBuilder& {
            context.assign_to_message(m_diagnostic);
            return *this;
        }

    private:
        friend class SubDiagnosticBuilder<LocT, DiagnosticKindType>;
        friend class BasicDiagnosticEmitter<LocT, DiagnosticKindType>;
    private:
        template <typename... Args>
        DiagnosticBuilder(
            emitter_t* emitter,
            LocT loc,
            DiagnosticLevel level,
            DiagnosticBase<DiagnosticKindType, Args...> const& base,
            core::Formatter formatter
        )
            : m_emitter(emitter)
        {
            assert((level != DiagnosticLevel::Note) && "Note messages cannot be a base diagnostic level");
            add_message(std::move(loc), level, base, std::move(formatter));
        }

        template <typename... Args>
        auto add_message(
            LocT loc,
            DiagnosticLevel level,
            DiagnosticBase<DiagnosticKindType, Args...> const& base,
            core::Formatter formatter
        ) -> void {
            add_message_with_loc(
                level,
                m_emitter->m_converter->convert_loc(loc, *this),
                base,
                std::move(formatter)
            );
        }

        template <typename... Args>
        auto add_message_with_loc(
            DiagnosticLevel level,
            DiagnosticLocation location,
            DiagnosticBase<DiagnosticKindType, Args...> const& base,
            core::Formatter formatter
        ) -> void {
            m_diagnostic =  {
                std::move(base.kind),
                level,
                std::move(formatter),
                std::move(location)
            };
        }

    private:
        emitter_t* m_emitter;
        BasicDiagnostic<DiagnosticKindType> m_diagnostic;
    };

} // namespace dark::internal

#endif // DARK_DIAGNOSTICS_BUILDER_DIAGNOSTIC_HPP
