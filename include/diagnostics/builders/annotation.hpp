#ifndef AMT_DARK_DIAGNOSTICS_BUILDER_ANNOTATION_HPP
#define AMT_DARK_DIAGNOSTICS_BUILDER_ANNOTATION_HPP

#include "diagnostic.hpp"
#include "../span.hpp"
#include <array>

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

        /**
         * @brief Note annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @tparam Ss The varidaic pack of spans marking different tokens.
         * @param spans The variadic argument of Span
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto note(
            core::CowString message,
            Ss&&... spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        /**
         * @brief Note annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @tparam Ss The varidaic pack of spans marking different tokens.
         * @param spans The variadic argument of Span
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto note(
            term::AnnotatedString message,
            Ss&&... spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        /**
         * @brief Note annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param spans The collection of spans
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto note(
            core::CowString message,
            std::span<Span> spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        /**
         * @brief Note annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param spans The collection of spans
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto note(
            term::AnnotatedString message,
            std::span<Span> spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        /**
         * @brief Note annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param suggestion The collection of tokens that will be rendered at the end of the diagnostics.
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto note(
            core::CowString message,
            DiagnosticSourceLocationTokens suggestion
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }

        /**
         * @brief Note annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param suggestion The collection of tokens that will be rendered at the end of the diagnostics.
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto note(
            term::AnnotatedString message,
            DiagnosticSourceLocationTokens suggestion
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Note,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }


        // MARK: Error

        /**
         * @brief Error annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @tparam Ss The varidaic pack of spans marking different tokens.
         * @param spans The variadic argument of Span
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto error(
            core::CowString message,
            Ss&&... spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        /**
         * @brief Error annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @tparam Ss The varidaic pack of spans marking different tokens.
         * @param spans The variadic argument of Span
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto error(
            term::AnnotatedString message,
            Ss&&... spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        /**
         * @brief Error annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param spans The collection of spans
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto error(
            core::CowString message,
            std::span<Span> spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        /**
         * @brief Error annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param spans The collection of spans
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto error(
            term::AnnotatedString message,
            std::span<Span> spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        /**
         * @brief Error annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param suggestion The collection of tokens that will be rendered at the end of the diagnostics.
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto error(
            core::CowString message,
            DiagnosticSourceLocationTokens suggestion
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }

        /**
         * @brief Error annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param suggestion The collection of tokens that will be rendered at the end of the diagnostics.
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto error(
            term::AnnotatedString message,
            DiagnosticSourceLocationTokens suggestion
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Error,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }


        // MARK: Help

        /**
         * @brief Help annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @tparam Ss The varidaic pack of spans marking different tokens.
         * @param spans The variadic argument of Span
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto help(
            core::CowString message,
            Ss&&... spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Help,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        /**
         * @brief Help annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @tparam Ss The varidaic pack of spans marking different tokens.
         * @param spans The variadic argument of Span
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto help(
            term::AnnotatedString message,
            Ss&&... spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Help,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        /**
         * @brief Help annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param spans The collection of spans
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto help(
            core::CowString message,
            std::span<Span> spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Help,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        /**
         * @brief Help annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param spans The collection of spans
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto help(
            term::AnnotatedString message,
            std::span<Span> spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Help,
                std::move(message),
                {},
                spans
            );
            return *this;
        }


        /**
         * @brief Help annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param suggestion The collection of tokens that will be rendered at the end of the diagnostics.
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto help(
            core::CowString message,
            DiagnosticSourceLocationTokens suggestion
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Help,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }

        /**
         * @brief Help annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param suggestion The collection of tokens that will be rendered at the end of the diagnostics.
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto help(
            term::AnnotatedString message,
            DiagnosticSourceLocationTokens suggestion
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Help,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }


        // MARK: Warn

        /**
         * @brief Warning annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @tparam Ss The varidaic pack of spans marking different tokens.
         * @param spans The variadic argument of Span
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto warn(
            core::CowString message,
            Ss&&... spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        /**
         * @brief Warning annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @tparam Ss The varidaic pack of spans marking different tokens.
         * @param spans The variadic argument of Span
         */
        template <detail::IsSpan... Ss>
        [[nodiscard("Missing `end_annotation()` call")]] auto warn(
            term::AnnotatedString message,
            Ss&&... spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(message),
                {},
                spans...
            );
            return *this;
        }

        /**
         * @brief Warning annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param spans The collection of spans
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto warn(
            core::CowString message,
            std::span<Span> spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        /**
         * @brief Warning annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param spans The collection of spans
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto warn(
            term::AnnotatedString message,
            std::span<Span> spans
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(message),
                {},
                spans
            );
            return *this;
        }

        /**
         * @brief Warning annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param suggestion The collection of tokens that will be rendered at the end of the diagnostics.
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto warn(
            core::CowString message,
            DiagnosticSourceLocationTokens suggestion
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }

        /**
         * @brief Warning annotation the will displayed for the marked span; otherwise rendered at the end of the diagnostics.
         * @param message The message for marked token.
         * @param suggestion The collection of tokens that will be rendered at the end of the diagnostics.
         */
        [[nodiscard("Missing `end_annotation()` call")]] auto warn(
            term::AnnotatedString message,
            DiagnosticSourceLocationTokens suggestion
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Warning,
                std::move(message),
                std::move(suggestion)
            );
            return *this;
        }

        // MARK: Insert

        /**
         * @brief Inserts the string starting at offset and ends with offset + string size. It doesn't care about token boundary.
         * @param text The text to be inserted
         * @param offset The offset starting from the start of the source text.
         * @param message The message to displayed for the inserted text.
         */
        auto insert(
            core::CowString text,
            dsize_t offset,
            core::CowString message = {}
        ) -> DiagnosticAnnotationBuilder {
            std::println("INSERT");
            auto span = Span::from_size(offset, static_cast<dsize_t>(text.size()));
            annotate_helper(
                DiagnosticLevel::Insert,
                std::move(message),
                DiagnosticSourceLocationTokens::builder()
                    .add_text(std::move(text), 0, 0, 0, span.size())
                .build(),
                span
            );
            return *this;
        }

        /**
         * @brief Inserts the string starting at offset and ends with offset + string size. It doesn't care about token boundary.
         * @param text The text to be inserted
         * @param offset The offset starting from the start of the source text.
         * @param message The message to displayed for the inserted text.
         */
        auto insert(
            core::CowString text,
            dsize_t offset,
            AnnotatedString message
        ) -> DiagnosticAnnotationBuilder {
            auto span = Span::from_size(offset, static_cast<dsize_t>(text.size()));
            annotate_helper(
                DiagnosticLevel::Insert,
                std::move(message),
                DiagnosticSourceLocationTokens::builder()
                    .add_text(std::move(text), 0, 0, 0, span.size())
                .build(),
                span
            );
            return *this;
        }

        /**
         * @brief Inserts the string starting at offset and ends with offset + string size. It doesn't care about token boundary.
         * @param tokens The tokens to be inserted
         * @param offset The offset starting from the start of the source text.
         * @param message The message to displayed for the inserted tokens.
         */
        auto insert(
            DiagnosticSourceLocationTokens tokens,
            dsize_t offset,
            core::CowString message = {}
        ) -> DiagnosticAnnotationBuilder {
            auto span = Span::from_size(offset, tokens.rel_span().size());
            annotate_helper(
                DiagnosticLevel::Insert,
                std::move(message),
                std::move(tokens),
                span
            );
            return *this;
        }

        /**
         * @brief Inserts the string starting at offset and ends with offset + string size. It doesn't care about token boundary.
         * @param tokens The tokens to be inserted
         * @param offset The offset starting from the start of the source text.
         * @param message The message to displayed for the inserted tokens.
         */
        auto insert(
            DiagnosticSourceLocationTokens tokens,
            dsize_t offset,
            AnnotatedString message
        ) -> DiagnosticAnnotationBuilder {
            auto span = Span::from_size(offset, tokens.rel_span().size());
            annotate_helper(
                DiagnosticLevel::Insert,
                std::move(message),
                std::move(tokens),
                span
            );
            return *this;
        }

        /**
         * @brief Inserts the string starting at offset and ends with offset + string size. It doesn't care about token boundary.
         * @param tokens The tokens to be inserted
         * @param offset The offset starting from the start of the source text.
         * @param message The message to displayed for the inserted tokens.
         */
        auto insert(
            DiagnosticSourceLocationTokens tokens,
            core::CowString message = {}
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Insert,
                std::move(message),
                std::move(tokens),
                tokens.span()
            );
            return *this;
        }

        /**
         * @brief Inserts the string starting at offset and ends with offset + string size. It doesn't care about token boundary.
         * @param tokens The tokens to be inserted
         * @param offset The offset starting from the start of the source text.
         * @param message The message to displayed for the inserted tokens.
         */
        auto insert(
            DiagnosticSourceLocationTokens tokens,
            AnnotatedString message
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Insert,
                std::move(message),
                std::move(tokens),
                tokens.span()
            );
            return *this;
        }

        // MARK: Delete

        /**
         * @brief Inserts the string starting at offset and ends with offset + string size. It doesn't care about token boundary.
         * @param message The message to displayed for the removed tokens.
         * @param span The span to the text to be removed.
         */
        auto remove(
            core::CowString message,
            Span span
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Delete,
                std::move(message),
                {},
                span
            );
            return *this;
        }

        /**
         * @brief Inserts the string starting at offset and ends with offset + string size. It doesn't care about token boundary.
         * @param message The message to displayed for the removed tokens.
         * @param span The span to the text to be removed.
         */
        auto remove(
            AnnotatedString message,
            Span span
        ) -> DiagnosticAnnotationBuilder {
            annotate_helper(
                DiagnosticLevel::Delete,
                std::move(message),
                {},
                span
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
            std::array<Span, sizeof...(Ss)> tmp = { spans... };
            annotate_helper(level, std::move(an), std::move(tokens), std::span(tmp));
        }

        auto annotate_helper(
            DiagnosticLevel level,
            term::AnnotatedString&& an,
            DiagnosticSourceLocationTokens tokens,
            std::span<Span> spans
        ) -> void {
            auto& diag = m_builder->m_diagnostic;
            auto const& source = diag.location.source;
            auto marker = source.marker();
            if (marker) {
                for (auto& s: spans) {
                    if (s.empty() && s.start() == 0) {
                        s = *marker;
                    } 
                }
            }

            auto& annotations = diag.annotations;
            annotations.push_back(DiagnosticMessage {
                .message = std::move(an),
                .tokens = std::move(tokens),
                .spans = { spans },
                .level = level
            });
        }

        auto annotate_helper(
            DiagnosticLevel level,
            term::AnnotatedString&& an,
            DiagnosticSourceLocationTokens tokens,
            Span span
        ) -> void {
            annotate_helper(level, std::move(an), std::move(tokens), std::span(&span, 1));
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
    private:
        DiagnosticBuilder<LocT>* m_builder;
    };

    template <typename LocT>
    inline constexpr auto DiagnosticBuilder<LocT>::begin_annotation() noexcept -> DiagnosticAnnotationBuilder<LocT> {
        return {*this};
    }
} // namespace dark::builder

#endif // AMT_DARK_DIAGNOSTICS_BUILDER_ANNOTATION_HPP
