#ifndef DARK_DIAGNOSTICS_LINE_CONVERTER_HPP
#define DARK_DIAGNOSTICS_LINE_CONVERTER_HPP

#include "basic.hpp"
#include "diagnostics/core/function_ref.hpp"
#include <cstddef>
#include <variant>

namespace dark {

    namespace internal {
        template <typename LocT, typename DiagnosticKindType>
        class SubDiagnosticBuilder;

        template <typename LocT, typename DiagnosticKindType>
        class DiagnosticBuilder;

    } // namespace internal

    template <typename LocT, typename DiagnosticKindType = EmptyDiagnosticKind>
    struct BasicDiagnosticConverter {
    private:
        using sub_diag_t = internal::SubDiagnosticBuilder<LocT, DiagnosticKindType>;
        using diag_t = internal::DiagnosticBuilder<LocT, DiagnosticKindType>;

        struct BuilderContext {
            constexpr BuilderContext() noexcept = default;
            constexpr BuilderContext(BuilderContext const&) noexcept = default;
            constexpr BuilderContext& operator=(BuilderContext const&) noexcept = default;
            constexpr BuilderContext(BuilderContext&&) noexcept = default;
            constexpr BuilderContext& operator=(BuilderContext&&) noexcept = default;
            constexpr ~BuilderContext() noexcept = default;

            constexpr BuilderContext(sub_diag_t& diag) noexcept
                : m_is_sub_diagnostic(true)
            {
                m_data.sub_diag = &diag;
            }

            constexpr BuilderContext(diag_t& diag) noexcept
                : m_is_sub_diagnostic(false)
            {
                m_data.diag = &diag;
            }

            constexpr auto is_sub_diagnostic_builder() const noexcept -> bool { return m_is_sub_diagnostic; }
            constexpr auto is_diagnostic_builder() const noexcept -> bool { return !m_is_sub_diagnostic; }

            constexpr auto as_sub_diagnostic_builder() const noexcept -> sub_diag_t& { return *m_data.sub_diag; }
            constexpr auto as_diagnostic_builder() const noexcept -> diag_t& { return *m_data.diag; }

        private:
            union {
                sub_diag_t* sub_diag;
                diag_t* diag;
            } m_data { nullptr };
            bool m_is_sub_diagnostic {false};
        };
    public:
        using loc_t = LocT;

        using builder_t = BuilderContext;
        virtual ~BasicDiagnosticConverter() = default;
        virtual auto convert_loc(LocT loc, builder_t builder) const -> DiagnosticLocation = 0;
    };

} // namespace dark

#endif // DARK_DIAGNOSTICS_LINE_CONVERTER_HPP
