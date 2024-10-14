#include <cstdio>
#include "diagnostics.hpp"

enum class DiagnosticKind {
    InvalidFunctionDefinition,
    InvalidFunctionPrototype
};

template <typename LocT>
using DiagnosticConverter = dark::BasicDiagnosticConverter<LocT, DiagnosticKind>;

struct Info {
    unsigned line{};
    unsigned col{};
};

struct SimpleConverter: DiagnosticConverter<Info> {

    enum class TokenKind {
        Identifier,
        Keyword,
        Puncutation,
    };

    auto get_token(TokenKind kind, dark::core::CowString text, unsigned offset) const {
        switch (kind) {
            case TokenKind::Keyword:  return dark::BasicDiagnosticTokenItem {
                .token = std::move(text),
                .column_number = offset,
                .color = dark::Stream::GREEN,
                .is_bold = true
            };
            case TokenKind::Identifier:  return dark::BasicDiagnosticTokenItem {
                .token = std::move(text),
                .column_number = offset,
            };
            case TokenKind::Puncutation:  return dark::BasicDiagnosticTokenItem {
                .token = std::move(text),
                .column_number = offset,
                .color = dark::Stream::BLUE,
                .is_bold = true
            };
        }
    }

    auto convert_loc(Info loc, [[maybe_unused]] builder_t builder) const -> dark::DiagnosticLocation override {
        auto line1 = dark::LineDiagnosticToken { .tokens = {}, .line_number = loc.line, .source_location = 1 };
        auto offset = loc.col + 1;
        line1.tokens.push_back(get_token(TokenKind::Keyword, "int", offset));
        offset += static_cast<unsigned>(line1.tokens.back().token.size() + 1);
       
        line1.tokens.push_back(get_token(TokenKind::Identifier, "test", offset));
        offset += static_cast<unsigned>(line1.tokens.back().token.size() + 1);

        line1.tokens.push_back(get_token(TokenKind::Puncutation, "(", offset));
        offset += static_cast<unsigned>(line1.tokens.back().token.size() + 1);
        
        line1.tokens.push_back(get_token(TokenKind::Keyword, "int", offset));
        offset += static_cast<unsigned>(line1.tokens.back().token.size() + 1);
        
        line1.tokens.push_back(get_token(TokenKind::Identifier, "a", offset));
        offset += static_cast<unsigned>(line1.tokens.back().token.size() + 1);

        line1.tokens.push_back(get_token(TokenKind::Puncutation, ",", offset));
        offset += static_cast<unsigned>(line1.tokens.back().token.size() + 1);
        
        line1.tokens.push_back(get_token(TokenKind::Keyword, "int", offset));
        offset += static_cast<unsigned>(line1.tokens.back().token.size() + 1);
        
        line1.tokens.push_back(get_token(TokenKind::Identifier, "b", offset));
        offset += static_cast<unsigned>(line1.tokens.back().token.size() + 1);

        line1.tokens.push_back(get_token(TokenKind::Puncutation, ")", offset));
        
        return dark::DiagnosticLocation {
            .filename = "test.cpp",
            .source = dark::DiagnosticLocationTokens {
                .lines = {
                    line1
                },
                .marker = dark::LocRelSpan::from_size(0, 2).resolve(1)
            },
        };
    }
};

int main() {

    /*auto stream = dark::Stream(stdout, dark::StreamColorMode::Enable);*/
    auto stream = dark::out();
    auto consumer = dark::BasicStreamDiagnosticConsumer<DiagnosticKind>(stream);
    auto converter = SimpleConverter();
    auto emitter = dark::BasicDiagnosticEmitter(
        converter,
        consumer
    );

    static constexpr auto InvalidFunctionDefinition = dark_make_diagnostic_with_kind(
        DiagnosticKind::InvalidFunctionDefinition,
        "Invalid function definition for {s} at {u32}"
    );

    static constexpr auto InvalidFunctionPrototype = dark_make_diagnostic_with_kind(
        DiagnosticKind::InvalidFunctionPrototype,
        "The prototype is defined here"
    );

    emitter
        .error({1, 1}, InvalidFunctionDefinition, "Test", 0u)
        .context(
            dark::DiagnosticContext()
                .insert(")", 2)
                .insert_marker_rel(")", 3)
                .del(dark::MarkerRelSpan(4, 8))
                .error(
                    "prototype does not match the defination",
                    dark::LocRelSpan(0, 2), // start + source_location
                    dark::Span(19, 24)
                )
                .warn(dark::Span(6, 10), dark::Span(25, 27))
                .note("Try to fix the error")
        )
        .sub_diagnostic()
            .warn({0, 1}, InvalidFunctionPrototype)
        .build()
        .emit();

    return 0;
}
