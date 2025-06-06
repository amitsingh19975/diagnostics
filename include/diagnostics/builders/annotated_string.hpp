#ifndef AMT_DARK_DIAGNOSTICS_BUILDERS_ANNOTATED_STRING_HPP
#define AMT_DARK_DIAGNOSTICS_BUILDERS_ANNOTATED_STRING_HPP

namespace dark::builder {
    struct AnnotatedStringBuilder {
        struct WithStyleBuilder;

        AnnotatedStringBuilder() noexcept = default;
        AnnotatedStringBuilder(AnnotatedStringBuilder const&) noexcept = default;
        AnnotatedStringBuilder(AnnotatedStringBuilder &&) noexcept = default;
        AnnotatedStringBuilder& operator=(AnnotatedStringBuilder const&) noexcept = default;
        AnnotatedStringBuilder& operator=(AnnotatedStringBuilder &&) noexcept = default;
        ~AnnotatedStringBuilder() noexcept = default;

        [[nodiscard("Missing 'build()' call")]] auto push(core::CowString s, term::SpanStyle style = {}) -> AnnotatedStringBuilder& {
            m_an.push(std::move(s), std::move(style));
            return *this;
        }

        [[nodiscard("Missing 'build()' call")]] constexpr auto with_style(term::SpanStyle style) -> WithStyleBuilder;

        auto build() -> term::AnnotatedString&& { return std::move(m_an); }
    private:
        term::AnnotatedString m_an;
    };

    struct AnnotatedStringBuilder::WithStyleBuilder {
        constexpr WithStyleBuilder(
            term::SpanStyle style,
            AnnotatedStringBuilder& builder
        ) noexcept
            : m_style(std::move(style))
            , m_builder(&builder)
        {}
        constexpr WithStyleBuilder(WithStyleBuilder const&) noexcept = default;
        constexpr WithStyleBuilder(WithStyleBuilder &&) noexcept = default;
        constexpr WithStyleBuilder& operator=(WithStyleBuilder const&) noexcept = default;
        constexpr WithStyleBuilder& operator=(WithStyleBuilder &&) noexcept = default;
        constexpr ~WithStyleBuilder() noexcept = default;

        [[nodiscard("Missing 'build()' call")]] auto push(core::CowString s) -> WithStyleBuilder {
            m_builder->m_an.push(std::move(s), m_style);
            return *this;
        }

       [[nodiscard("Missing 'build()' call")]]  auto with_style(term::SpanStyle style) -> WithStyleBuilder {
            return { std::move(style), *m_builder };
        }

        auto build() -> term::AnnotatedString&& { return std::move(m_builder->m_an); }
    private:
        term::SpanStyle m_style;
        AnnotatedStringBuilder* m_builder;
    };

    inline constexpr auto AnnotatedStringBuilder::with_style(term::SpanStyle style) -> WithStyleBuilder {
        return { std::move(style), *this };
    }
} // namespace dark::builder

#endif // AMT_DARK_DIAGNOSTICS_BUILDERS_ANNOTATED_STRING_HPP
