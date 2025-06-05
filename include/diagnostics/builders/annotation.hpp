#ifndef AMT_DARK_DIAGNOSTICS_BUILDER_ANNOTATION_HPP
#define AMT_DARK_DIAGNOSTICS_BUILDER_ANNOTATION_HPP

#include "diagnostic.hpp"
#include "../span.hpp"
#include "diagnostics/basic.hpp"

namespace dark::builder {
    template <typename LocT>
    struct DiagnosticAnnotationBuilder {
        constexpr DiagnosticAnnotationBuilder(
            DiagnosticBuilder<LocT>& builder
        )
            : m_builder(&builder)
        {}
        constexpr DiagnosticAnnotationBuilder(DiagnosticAnnotationBuilder const&) noexcept = default;
        constexpr DiagnosticAnnotationBuilder(DiagnosticAnnotationBuilder &&) noexcept = default;
        constexpr DiagnosticAnnotationBuilder& operator=(DiagnosticAnnotationBuilder const&) noexcept = default;
        constexpr DiagnosticAnnotationBuilder& operator=(DiagnosticAnnotationBuilder &&) noexcept = default;
        constexpr ~DiagnosticAnnotationBuilder() noexcept = default;

        [[nodiscard("Missing `emit()` call")]] constexpr auto end_annotation() noexcept -> DiagnosticBuilder<LocT>& {
            return *m_builder;
        }

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto note(core::CowString message, Ss&&... spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto note(core::CowString message, DiagnosticSourceLocationTokens suggestion) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto error(core::CowString message, Ss&&... spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto error(core::CowString message, DiagnosticSourceLocationTokens suggestion) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto remark(core::CowString message, Ss&&... spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Remark,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto remark(core::CowString message, DiagnosticSourceLocationTokens suggestion) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Remark,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto warn(core::CowString message, Ss&&... spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto warn(core::CowString message, DiagnosticSourceLocationTokens suggestion) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }


    private:
        template <detail::IsSpan... Ss>
        auto annotate_helper(
            DiagnosticLevel level,
            core::CowString message,
            DiagnosticSourceLocationTokens tokens,
            Ss&&... spans
        ) -> void {
            m_builder->m_diagnostic.annotations.push_back(DiagnosticMessage {
                .message = std::move(message),
                .tokens = std::move(tokens),
                .spans = { spans... },
                .level = level
            });
        }
    private:
        DiagnosticBuilder<LocT>* m_builder;
    };

    template <typename LocT>
    inline constexpr auto DiagnosticBuilder<LocT>::begin_annotation() noexcept -> DiagnosticAnnotationBuilder<LocT> {
        return {*this};
    }
} // namespace dark::builder

#endif // AMT_DARK_DIAGNOSTICS_BUILDER_ANNOTATION_HPP
