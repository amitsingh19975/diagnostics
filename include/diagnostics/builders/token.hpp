#ifndef AMT_DARK_DIAGNOSTICS_BUILDER_TOKEN_HPP
#define AMT_DARK_DIAGNOSTICS_BUILDER_TOKEN_HPP

#include "../forward.hpp"
#include "../core/cow_string.hpp"
#include "../core/term/color.hpp"

namespace dark::builder {
    struct DiagnosticTokenBuilder {
        struct DiagnosticLineTokenBuilder;

        constexpr DiagnosticTokenBuilder() noexcept = default;
        constexpr DiagnosticTokenBuilder(DiagnosticTokenBuilder const&) noexcept = delete;
        constexpr DiagnosticTokenBuilder(DiagnosticTokenBuilder &&) noexcept = default;
        constexpr DiagnosticTokenBuilder& operator=(DiagnosticTokenBuilder const&) noexcept = delete;
        constexpr DiagnosticTokenBuilder& operator=(DiagnosticTokenBuilder &&) noexcept = default;
        constexpr ~DiagnosticTokenBuilder() noexcept = default;

        /**
         * @brief Adds a new line for tokens.
         * @param line_number line number starts with 1 (1-based index).
         * @param line_start_offset offset to the line start from the start of the source text.
         * @return line builder which can be used to add tokens or text.
        */
        [[nodiscard("Missing `end_line()` call")]] auto begin_line(dsize_t line_number, dsize_t line_start_offset) -> DiagnosticLineTokenBuilder;

        /**
         * @brief Adds a new text. It'll parse newlines if present.
         * @param line_number line number starts with 1 (1-based index).
         * @param line_start_offset offset to the line start from the start of the source text.
         * @param column_number column number where the marker starts. It start from 1 (1-based index)
         * @param marker_len length of the marker; (column_number - 1, marker_len]
         */
        [[nodiscard("Missing `build()` call")]] auto add_text(
            std::string_view text,
            dsize_t line_number,
            dsize_t line_start_offset,
            dsize_t column_number,
            dsize_t marker_len
        ) -> DiagnosticTokenBuilder& {
            auto it = text.find("\n");
            do {
                auto txt = text.substr(0, it);
                auto newline_offset = dsize_t{};
                if (it != std::string_view::npos) {
                    text = text.substr(it + 1);
                    it = text.find("\n");
                    newline_offset = 1;
                } else {
                    text = "";
                }

                auto marker = Span::from_size(
                    line_start_offset + std::max(column_number, dsize_t{1}) - 1,
                    std::min(marker_len, static_cast<dsize_t>(txt.size()))
                );

                auto txt_size = marker.size() + newline_offset;
                marker_len = std::max(txt_size, marker_len) - txt_size;

                m_tokens.lines.push_back({
                    .tokens = {
                        DiagnosticTokenInfo {
                            .text = txt,
                            .column_number = column_number,
                            .marker = marker
                        }
                    },
                    .line_number = line_number,
                    .line_start_offset = line_start_offset
                });

                ++line_number;
                column_number = 0;
                //...\n[line_start_offset]....
                line_start_offset += txt.size() + newline_offset;
            } while (!text.empty());

            return *this;
        }

        [[nodiscard("Returns tokens")]] auto build() noexcept -> DiagnosticSourceLocationTokens&& {
            return std::move(m_tokens);
        }
    private:
        DiagnosticSourceLocationTokens m_tokens;
    };

    struct DiagnosticTokenBuilder::DiagnosticLineTokenBuilder {
        constexpr DiagnosticLineTokenBuilder(
            std::size_t line_index,
            DiagnosticTokenBuilder& m_builder
        )
            : m_line_index(line_index)
            , m_builder(&m_builder)
        {}
        constexpr DiagnosticLineTokenBuilder(DiagnosticLineTokenBuilder const&) noexcept = default;
        constexpr DiagnosticLineTokenBuilder(DiagnosticLineTokenBuilder &&) noexcept = default;
        constexpr DiagnosticLineTokenBuilder& operator=(DiagnosticLineTokenBuilder const&) noexcept = default;
        constexpr DiagnosticLineTokenBuilder& operator=(DiagnosticLineTokenBuilder &&) noexcept = default;

        #ifndef NDEBUG
        ~DiagnosticLineTokenBuilder() noexcept {
            assert(m_has_ended && "`end_line` is missing after call of `begin_line`");
        }
        #else
        ~DiagnosticLineTokenBuilder() noexcept = default;
        #endif

        /**
         * @brief Adds a new line for tokens.
         * @param line_number line number starts with 1 (1-based index).
         * @param line_start_offset offset to the line start from the start of the source text.
         * @return line builder which can be used to add tokens or text.
        */
        [[nodiscard("Missing `end_line()` call")]] auto begin_line(dsize_t line_number, dsize_t line_start_offset) -> DiagnosticLineTokenBuilder{
            (void)end_line();
            return m_builder->begin_line(line_number, line_start_offset);
        }

        /**
         * @brief Marks the end of line.
        */
        [[nodiscard("Missing `build()` call")]] auto end_line() -> DiagnosticTokenBuilder& {
            m_has_ended = true;
            return *m_builder;
        }

        /**
         * @brief Adds a token at the end of the current line.
         * @param column_number The column number of token from the start of the line. It start from 1 (1-based index)
         * @param marker The absolute span, where start is from the start of the source text and size is the marker length.
         * @param text_color The color of the token.
         * @param bg_color The background color of the token.
         * @param bold `true` if the token will be rendered as bold; otherwise `false`.
         * @param italic `true` if the token will be rendered as italic; otherwise `false`.
         */
        [[nodiscard("Missing `end_line()` call")]] auto add_token(
            core::CowString text,
            dsize_t column_number,
            Span marker = {},
            Color text_color = Color::Current,
            Color bg_color = Color::Current,
            bool bold = false,
            bool italic = false
        ) -> DiagnosticLineTokenBuilder {
            m_builder->m_tokens.lines[m_line_index].tokens.emplace_back(
                std::move(text),
                column_number,
                marker,
                text_color,
                bg_color,
                bold,
                italic
            );
            return *this;
        }

        [[nodiscard("Missing `end_line()` call")]] auto add_token(
            DiagnosticTokenInfo const& token
        ) -> DiagnosticLineTokenBuilder {
            m_builder->m_tokens.lines[m_line_index].tokens.push_back(token);
            return *this;
        }

        [[nodiscard("Missing `end_line()` call")]] auto add_token(
            DiagnosticTokenInfo && token
        ) -> DiagnosticLineTokenBuilder {
            m_builder->m_tokens.lines[m_line_index].tokens.push_back(std::move(token));
            return *this;
        }

        [[nodiscard("Missing `end_line()` call")]] auto add_tokens(
            std::span<DiagnosticTokenInfo> tokens
        ) -> DiagnosticLineTokenBuilder {
            for (auto t: tokens) (void) add_token(t);
            return *this;
        }

        [[nodiscard("Missing `end_line()` call")]] auto consume_tokens(
            std::span<DiagnosticTokenInfo> tokens
        ) -> DiagnosticLineTokenBuilder {
            for (auto t: tokens) (void) add_token(std::move(t));
            return *this;
        }
    private:
        std::size_t m_line_index;
        DiagnosticTokenBuilder* m_builder;
        bool m_has_ended{false};
    };

    inline auto DiagnosticTokenBuilder::begin_line(dsize_t line_number, dsize_t line_start_offset) -> DiagnosticLineTokenBuilder {
        if (m_tokens.lines.empty()) {
            m_tokens.lines.push_back({ .tokens = {}, .line_number = line_number, .line_start_offset = line_start_offset });
        }

        return DiagnosticLineTokenBuilder(m_tokens.lines.size() - 1, *this);
    }
} // namespace dark::builder
#endif // AMT_DARK_DIAGNOSTICS_BUILDER_TOKEN_HPP
