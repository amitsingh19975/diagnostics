#include <cstdint>
#include <cstdio>
#include <string_view>
#include "diagnostics.hpp"
#include "diagnostics/consumers/stream.hpp"
#include "diagnostics/emitter.hpp"

using namespace dark;

struct DiagnosticKind {
    static constexpr std::size_t InvalidFunctionDefinition = 1;
    static constexpr std::size_t InvalidFunctionPrototype = 2;
};

struct TokenConverter: DiagnosticConverter<Span> {
private:
    struct Info {
        dsize_t line_number{};
        dsize_t line_offset{};
    };

    struct Token {
        Span span;
        Color text_color{Color::Default};
        Color bg_color{Color::Default};
        bool bold{false};
        bool italic{false};

        static constexpr auto keyword(Span span) noexcept -> Token {
            return {
                .span = span,
                .text_color = Color::Blue,
                .bold = true
            };
        }

        static constexpr auto identifier(Span span) noexcept -> Token {
            return {
                .span = span,
                .text_color = Color(/*r=*/183, /*g=*/74, /*b=*/174)
            };
        }

        static constexpr auto punctuation(Span span) noexcept -> Token {
            return {
                .span = span,
                .text_color = Color::Green,
                .bold = true
            };
        }

        static constexpr auto whitespace(Span span) noexcept -> Token {
            return {
                .span = span,
            };
        }

        constexpr auto text(std::string_view src) const noexcept -> std::string_view {
            return src.substr(span.start(), span.size());
        }
    };

public:
    std::string_view source;
    std::string_view filename;
    std::vector<Token> tokens;

    TokenConverter(std::string_view source, std::string_view filename)
        : source(source)
        , filename(filename)
        , tokens(parse_tokens())
    {}

    auto convert_loc(loc_t loc, builder_t& /*builder*/) const -> DiagnosticLocation override {
        auto tks = DiagnosticSourceLocationTokens::builder();
        auto line_number = 1u;
        auto line_offset = 0u;
        auto builder = tks.begin_line(line_number, line_offset);
        for (auto const& token: tokens) {
            auto text = token.text(source);
            if (text[0] == '\n') {
                line_number += 1;
                line_offset = token.span.start();
                builder = builder
                    .end_line()
                    .begin_line(line_number, line_offset);
                continue;
            }
            auto marker = Span{};
            if (token.span.intersects(loc)) {
                marker = Span(loc.start(), std::min(loc.end(), token.span.end()));
                loc = Span(marker.end(), loc.end());
            }
            (void)builder.add_token(
                text,
                token.span.start(),
                marker,
                token.text_color,
                token.bg_color,
                token.bold,
                token.italic
            );
        }
        return DiagnosticLocation(filename, builder.end_line().build());
    }
private:
    constexpr auto parse_tokens() const noexcept -> std::vector<Token> {
        auto res = std::vector<Token>();

        for (auto i = 0u; i < source.size();) {
            while (i < source.size() && std::isspace(source[i])) {
                ++i;
            }

            {
                auto start = i;
                auto end = i;
                while (i < source.size() && std::isalnum(source[i])) {
                    ++i;
                    end = i;
                }
                if (start != end) {
                    auto text = source.substr(start, end - start);
                    if (text == "void" || text == "int") {
                        res.push_back(Token::keyword(Span(start, end)));
                    } else {
                        res.push_back(Token::identifier(Span(start, end)));
                    }
                    continue;
                }
            }

            if (i < source.size()) {
                res.push_back(Token::punctuation(Span::from_size(i, 1)));
                ++i;
            }
        }

        return res;
    }
};


int main() {

    auto consumer = ConsoleDiagnosticConsumer();
    auto converter = TokenConverter("void test( int a, int c );", "test.cpp");
    auto emitter = dark::DiagnosticEmitter(
        &converter,
        consumer
    );

    static constexpr auto InvalidFunctionDefinition = dark_make_diagnostic(
        DiagnosticKind::InvalidFunctionDefinition,
        "Invalid function definition for {} at {}",
        char const*, std::uint32_t
    );

    static constexpr auto InvalidFunctionPrototype = dark_make_diagnostic(
        DiagnosticKind::InvalidFunctionPrototype,
        "The prototype is defined here"
    );

    emitter
        .error(Span(0, 3), InvalidFunctionDefinition, "Test", 0u)
            .begin_annotation()
                .insert(")", 2)
                .remove(Span(4, 8))
                .error(
                    "prototype does not match the defination",
                    Span(0, 2),
                    Span(19, 24)
                )
                .warn(Span(6, 10), Span(25, 27))
                .note("Try to fix the error")
            .end_annotation()
        .emit();

    emitter
        .error(Span::from_size(5, 2), InvalidFunctionPrototype)
        .emit();

    return 0;
}
