#include "diagnostics.hpp"
#include "diagnostics/basic.hpp"
#include "diagnostics/consumer.hpp"
#include "diagnostics/core/stream.hpp"
#include "diagnostics/core/string_utils.hpp"
#include "diagnostics/emitter.hpp"
#include "diagnostics/span.hpp"
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

enum class DiagnosticKind {
    InvalidFunctionDefinition,
    InvalidFunctionPrototype
};

template <typename LocT>
using DiagnosticConverter = dark::BasicDiagnosticConverter<LocT, DiagnosticKind>;

struct SingleLineConverter: DiagnosticConverter<unsigned> {
    std::string filename{ "test.cpp" };
    std::string source{};
    unsigned absolute_position{};

    auto convert_loc(unsigned loc, [[maybe_unused]] builder_t builder) const -> dark::DiagnosticLocation override {
        auto [line, col] = get_loc_number(loc);
        return dark::DiagnosticLocation {
            .filename = filename,
            .source = dark::BasicDiagnosticLocationItem {
                .source = source,
                .line_number = line,
                .column_number = col,
                .source_location = absolute_position,
                .length = 1
            },

        };
    }
private:
    constexpr auto get_loc_number(unsigned loc) const noexcept -> std::pair<unsigned, unsigned> {
        unsigned line {1};
        unsigned col  {0};
        unsigned last_line_index{0};
        if (source.size() == 0) return { 0, 0 };
        for (auto i = 0zu; i < source.size(); ++i) {
            if (source[i] == '\n') {
                ++line;
                last_line_index = i;
            }
            if (loc == i) {
                col = i - last_line_index;
                break;
            }
        }
        return { line, col + 1 };
    }
};


struct Info {
    unsigned line{};
    unsigned col{};
};

struct MultilineConverter: DiagnosticConverter<Info> {
     std::string filename{ "test.cpp" };
     std::string source{};

     auto convert_loc(Info loc, [[maybe_unused]] builder_t builder) const -> dark::DiagnosticLocation override {
        return dark::DiagnosticLocation {
            .filename = filename,
            .source = dark::BasicDiagnosticLocationItem {
                // .source = "void test( int a, int c ); Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.",
                // .source = "void test( int a, int c );",
                .source = dark::core::utils::trim(R"(
void test( int a,
           int b,
           int c
        ))"),
                .line_number = loc.line + 1,
                .column_number = loc.col + 1,
                .source_location = 0,
                .length = 2
            }
        };
     }
 };

struct TokenConverter: DiagnosticConverter<Info> {

    std::size_t lines {1};

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

    auto make_line(Info loc, unsigned& abs_pos) const -> dark::LineDiagnosticToken {
        auto line = dark::LineDiagnosticToken { .tokens = {}, .line_number = loc.line + 1, .source_location = abs_pos };
        auto offset = loc.col + 1;
        line.tokens.push_back(get_token(TokenKind::Keyword, "int", offset));
        offset += static_cast<unsigned>(line.tokens.back().token.size() + 1);
       
        line.tokens.push_back(get_token(TokenKind::Identifier, "test", offset));
        offset += static_cast<unsigned>(line.tokens.back().token.size() + 1);

        line.tokens.push_back(get_token(TokenKind::Puncutation, "(", offset));
        offset += static_cast<unsigned>(line.tokens.back().token.size() + 1);
        
        line.tokens.push_back(get_token(TokenKind::Keyword, "int", offset));
        offset += static_cast<unsigned>(line.tokens.back().token.size() + 1);
        
        line.tokens.push_back(get_token(TokenKind::Identifier, "a", offset));
        offset += static_cast<unsigned>(line.tokens.back().token.size() + 1);

        line.tokens.push_back(get_token(TokenKind::Puncutation, ",", offset));
        offset += static_cast<unsigned>(line.tokens.back().token.size() + 1);
        
        line.tokens.push_back(get_token(TokenKind::Keyword, "int", offset));
        offset += static_cast<unsigned>(line.tokens.back().token.size() + 1);
        
        line.tokens.push_back(get_token(TokenKind::Identifier, "b", offset));
        offset += static_cast<unsigned>(line.tokens.back().token.size() + 1);

        line.tokens.push_back(get_token(TokenKind::Puncutation, ")", offset));
        offset += static_cast<unsigned>(line.tokens.back().token.size() + 1);
        abs_pos = offset + 4;
        return line;
    }

    auto convert_loc(Info loc, [[maybe_unused]] builder_t builder) const -> dark::DiagnosticLocation override {
        auto lines = std::max(1zu, this->lines);
        auto abs_pos = 0u;
        auto marker = dark::LocRelSpan::from_size(0, 2).resolve(abs_pos);
        auto temp = dark::DiagnosticLocationTokens {
            .lines = {
                make_line(loc, abs_pos)
            },
            .marker = marker 
        };
        
        for (auto i = 1; i < lines; ++i) {
            temp.lines.push_back(make_line({ .line = loc.line + i, .col = loc.col }, abs_pos));
        }

        return dark::DiagnosticLocation {
            .filename = "test.cpp",
            .source = std::move(temp) 
        };
    }
};


struct LineIterator {
    std::string source;
    std::size_t cursor{};

    [[nodiscard]] constexpr std::string_view next() noexcept {
        if (empty()) return {};
        auto it = source.find_first_of('\n', cursor);
        auto start = cursor;
        if (it == std::string::npos) {
            cursor = source.size();
            auto end = cursor;
            return std::string_view(source).substr(start, end - start);
        }
        auto end = it;
        cursor = it + 1;
        return std::string_view(source).substr(start, end - start);
    }

    constexpr auto empty() const noexcept -> bool {
        return cursor >= source.size();
    }

};

struct MockConsumer: dark::DiagnosticConsumer<DiagnosticKind> {
    auto consume(dark::BasicDiagnostic<DiagnosticKind>&& diagnostic) -> void override {
        consumer.consume(dark::BasicDiagnostic(diagnostic));
        diags.emplace_back(std::move(diagnostic));
    }

    void clear() {
        diags.clear();
        ss.clear();
    }

    [[nodiscard]] LineIterator as_line_iter()  const noexcept {
        return {
            .source = ss.str(),
        };
    }

    std::vector<dark::BasicDiagnostic<DiagnosticKind>> diags{};
    std::stringstream ss{};
    dark::Stream stream{ ss };
    dark::BasicStreamDiagnosticConsumer<DiagnosticKind> consumer { stream };
};

template <typename Converter>
struct Mock {
    MockConsumer consumer{};
    Converter converter{};
    dark::BasicDiagnosticEmitter<typename Converter::loc_t, DiagnosticKind> emitter{ converter, consumer };

    void clear() {
        consumer.clear();
        if constexpr(std::same_as<Converter, SingleLineConverter>) converter.absolute_position = 0;
    }

    [[nodiscard]] LineIterator as_line_iter() const noexcept {
        return consumer.as_line_iter();
    }

};
