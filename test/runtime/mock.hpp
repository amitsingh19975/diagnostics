#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "diagnostics.hpp"
#include "diagnostics/basic.hpp"
#include "diagnostics/core/string_utils.hpp"

struct DiagnosticKind {
    static constexpr std::size_t InvalidFunctionDefinition = 1;
    static constexpr std::size_t InvalidFunctionPrototype = 2;
};

using namespace dark;

struct SimpleConverter: DiagnosticConverter<Span> {
private:
    struct Info {
        unsigned line_number{};
        unsigned line_offset{};
    };

public:
    std::string_view source;
    std::string_view filename;

    SimpleConverter(std::string_view source, std::string_view filename)
        : source(source)
        , filename(filename)
    {}

    auto convert_loc(loc_t loc, builder_t& /*builder*/) const -> DiagnosticLocation override {
        auto info = get_info(loc);
        return DiagnosticLocation::from_text(
            filename,
            source,
            info.line_number,
            info.line_offset,
            loc.start(),
            loc
        );
    }
private:
    constexpr auto get_info(loc_t loc) const noexcept -> Info {
        if (source.empty()) return {};
        auto info = Info{ .line_number = 1 };
        for (auto i = 0ul; (i < source.size()) && i < loc.start(); ++i) {
            auto c = source[i];
            if (c == '\n') {
                info.line_number++;
                info.line_offset = i;
            }
        }
        return info;
    }
};

struct TokenConverter: DiagnosticConverter<Span> {
private:
    struct Info {
        unsigned line_number{};
        unsigned line_offset{};
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

        constexpr auto text(std::string_view source) const noexcept -> std::string_view {
            return source.substr(span.start(), span.size());
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
        auto line_number = 1;
        auto line_offset = 0;
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
                res.push_back(Token::whitespace(Span::from_size(i, 1)));
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

template <>
struct Writer<std::stringstream> {
    constexpr Writer(std::stringstream& os) noexcept
        : m_os(os)
    {}

    auto is_displayed() const noexcept -> bool {
        return false;
    }

    auto write(std::string_view str) -> void {
        m_os << str;
    }

    auto flush() noexcept -> void {
        m_os.flush();
    }

    auto columns() noexcept -> std::size_t {
        return 60;
    }

private:
    std::stringstream& m_os;
};

struct LineIterator{
    std::string str;
    std::size_t cursor{};

    constexpr auto next() noexcept -> std::string_view {
        if (empty()) return {};
        auto tmp = std::string_view(str);
        auto start = cursor;

        auto pos = tmp.find('\n', cursor);
        if (pos == std::string_view::npos) {
            cursor = std::string_view::npos;
            return dark::core::utils::rtrim(tmp.substr(start));
        }
        cursor = pos + 1;
        return dark::core::utils::rtrim(tmp.substr(start, pos - start));
    }

    constexpr auto empty() const noexcept -> bool {
        return cursor >= str.size();
    }
};

struct TestConsumer: DiagnosticConsumer {
    std::vector<Diagnostic> diagnostics;
    Terminal<std::stringstream> term;
    std::stringstream os;

    TestConsumer(TerminalColorMode mode = TerminalColorMode::Disable)
        : term(Writer<std::stringstream>(os), mode)
    {}

    auto consume(Diagnostic&& d) -> void override {
        diagnostics.push_back(std::move(d));
    }

    auto flush() -> void override {
        for (auto& d: diagnostics){
            render_diagnostic(term, d, {});
        }
    }

    constexpr auto line_iter() const noexcept -> LineIterator {
        return { .str = os.str() };
    }

    auto clear() -> void {
        diagnostics.clear();
        os.str("");
        os.clear();
    }
};

struct Mock {
    DiagnosticConverter<Span>* converter{};
    TestConsumer consumer{};
    DiagnosticEmitter<Span> emitter;

    Mock(
        DiagnosticConverter<Span>* converter
    )
        : converter(converter)
        , emitter(converter, &consumer)
    {}

    auto render_diagnostic() {
        consumer.flush();
    }

    constexpr auto line_iter() const noexcept -> LineIterator {
        return consumer.line_iter();
    }

    constexpr auto diagnostics() const noexcept -> std::span<const Diagnostic> {
        return consumer.diagnostics;
    }

    auto clear() -> void {
        consumer.clear();
    }
};
