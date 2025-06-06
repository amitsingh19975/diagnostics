#ifndef AMT_DARK_DIAGNOSTICS_BUILDER_ANNOTATION_HPP
#define AMT_DARK_DIAGNOSTICS_BUILDER_ANNOTATION_HPP

#include "diagnostic.hpp"
#include "../span.hpp"
#include "diagnostics/basic.hpp"
#include "diagnostics/builders/annotated_string.hpp"
#include "diagnostics/core/term/annotated_string.hpp"

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

        // MARK: Note section
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

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto note(term::AnnotatedString annotation, Ss&&... spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(annotation),
                {},
                spans...
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto note(core::CowString message, std::span<Span> spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto note(term::AnnotatedString annotation, std::span<Span> spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(annotation),
                {},
                spans
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

        [[nodiscard("Missing `end_annotation()` call")]] auto note(term::AnnotatedString annotation, DiagnosticSourceLocationTokens suggestion) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(annotation),
                std::move(suggestion)
            );
            return *this;
        }


        // MARK: Error
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

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto error(term::AnnotatedString annotation, Ss&&... spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(annotation),
                {},
                spans...
            );
            return *this;
        }

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto error(core::CowString message, std::span<Span> spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto error(term::AnnotatedString annotation, std::span<Span> spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(annotation),
                {},
                spans
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

        [[nodiscard("Missing `end_annotation()` call")]] auto error(term::AnnotatedString annotation, DiagnosticSourceLocationTokens suggestion) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(annotation),
                std::move(suggestion)
            );
            return *this;
        }


        // MARK: Remark
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

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto remark(term::AnnotatedString annotation, Ss&&... spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Remark,
                std::move(annotation),
                {},
                spans...
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto remark(core::CowString message, std::span<Span> spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Remark,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto remark(term::AnnotatedString annotation, std::span<Span> spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Remark,
                std::move(annotation),
                {},
                spans
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

        [[nodiscard("Missing `end_annotation()` call")]] auto remark(term::AnnotatedString annotation, DiagnosticSourceLocationTokens suggestion) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Remark,
                std::move(annotation),
                std::move(suggestion)
            );
            return *this;
        }


        // MARK: Warn
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

        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto warn(term::AnnotatedString annotation, Ss&&... spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(annotation),
                {},
                spans...
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto warn(core::CowString message, std::span<Span> spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        [[nodiscard("Missing `end_annotation()` call")]] auto warn(term::AnnotatedString annotation, std::span<Span> spans) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(annotation),
                {},
                spans
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

        [[nodiscard("Missing `end_annotation()` call")]] auto warn(term::AnnotatedString annotation, DiagnosticSourceLocationTokens suggestion) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(annotation),
                std::move(suggestion)
            );
            return *this;
        }


    private:
        template <detail::IsSpan... Ss>
        auto annotate_helper(
            DiagnosticLevel level,
            term::AnnotatedString&& an,
            DiagnosticSourceLocationTokens tokens,
            Ss&&... spans
        ) -> void {
            m_builder->m_diagnostic.annotations.push_back(DiagnosticMessage {
                .message = std::move(an),
                .tokens = std::move(tokens),
                .spans = { spans... },
                .level = level
            });
        }

        auto annotate_helper(
            DiagnosticLevel level,
            term::AnnotatedString&& an,
            DiagnosticSourceLocationTokens tokens,
            std::span<Span> spans
        ) -> void {
            m_builder->m_diagnostic.annotations.push_back(DiagnosticMessage {
                .message = std::move(an),
                .tokens = std::move(tokens),
                .spans = { spans },
                .level = level
            });
        }

        template <detail::IsSpan... Ss>
        auto annotate_helper(
            DiagnosticLevel level,
            core::CowString message,
            DiagnosticSourceLocationTokens tokens,
            Ss&&... spans
        ) -> void {
            annotate_helper(level, AnnotatedString::builder().push(std::move(message)).build(), std::move(tokens), spans...);
        }

        auto annotate_helper(
            DiagnosticLevel level,
            core::CowString message,
            DiagnosticSourceLocationTokens tokens,
            std::span<Span> spans
        ) -> void {
            annotate_helper(level, AnnotatedString::builder().push(std::move(message)).build(), std::move(tokens), spans);
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
