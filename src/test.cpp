#include <cstdio>
#include <iostream>
#include "diagnostics.hpp"
#include <cxxabi.h>

void print_type(char const* name) {
    int status;
    char* demangled_name = abi::__cxa_demangle(name, nullptr, nullptr, &status);

    if (status == 0) {
        std::cout << "Demangled name: " << demangled_name << std::endl;
        free(demangled_name);
    } else {
        std::cerr << "Demangling failed." << std::endl;
    }
}

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
/*        .source = dark::core::utils::trim(R"(*/
/*void test( int a,*/
/*           int b,*/
/*           int c*/
/*        ))"),*/
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
            .filename = "TEst.txt",
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

    auto stream = dark::Stream(stdout, dark::StreamColorMode::Enable);
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
        .error({ 22, 1 }, InvalidFunctionDefinition, "Test", 0u)
        .context(
            dark::DiagnosticContext()
                 .insert("]", 3)
                 .insert_marker_rel("))))", 1, "Missing parens.")
                 .del(dark::MarkerRelSpan(3, 7), "Remove this thing")
                .error(
                    "prototype does not match the defination Lorem ipsum dolor sit amet, consectetur adipisicing elit. Eligendi non quis exercitationem culpa nesciunt nihil aut nostrum explicabo reprehenderit optio amet ab temporibus asperiores quasi cupiditate. Voluptatum ducimus voluptates voluptas?",
                    dark::Span::from_usize(11, 5),
                    dark::Span::from_usize(28, 5),
                    dark::Span::from_usize(45, 5)
                )
                .warn(dark::Span::from_usize(13, 4))
                .warn("Testing warning.", dark::Span::from_usize(11, 5))
                // .warn(dark::Span(5, 6), dark::Span(6, 8))
                .note("Try to fix the error")
        )
        .sub_diagnostic()
            .error({222, 0}, InvalidFunctionPrototype)
        .build()
        .emit();

    // print_type(typeid(InvalidFunctionDefinition).name());

    return 0;
}
