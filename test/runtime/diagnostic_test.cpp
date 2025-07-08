#include "catch2/catch_message.hpp"
#include "diagnostics/basic.hpp"
#include "mock.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <memory>
#include <vector>

using namespace dark;

TEST_CASE("Diagnostic Builder", "[diagnostic:builder]") {
    auto simple_converter = std::make_unique<SimpleConverter>(
        "void test( int a, int c );",
        "main.cpp"
    );
    auto mock = Mock(simple_converter.get());

    {
        mock.clear();

        constexpr auto InvalidFunctionDefinition = dark_make_diagnostic(
            DiagnosticKind::InvalidFunctionDefinition,
            "Invalid function definition for {} at {}",
            std::string_view, std::uint32_t
        );

        constexpr auto InvalidFunctionPrototype = dark_make_diagnostic(
            DiagnosticKind::InvalidFunctionPrototype,
            "The prototype is defined here"
        );

        auto& emitter = mock.emitter;
        emitter
            .error(Span(1, 4), InvalidFunctionDefinition, std::string_view{"Test"}, 0u)
            .begin_annotation()
                .insert(")", 2)
                .remove(Span(3, 4))
                .error(
                    "prototype does not match the defination",
                    dark::Span(0, 2),
                    dark::Span(20, 21)
                )
                .warn(dark::Span(5, 10), dark::Span(25, 27))
                .note("Try to fix the error")
            .end_annotation()
            .emit();

        auto diagnostics = mock.diagnostics();
        REQUIRE(diagnostics.size() == 1);
        auto& diag = diagnostics[0];

        REQUIRE(diag.level == DiagnosticLevel::Error);

        REQUIRE(diag.level == DiagnosticLevel::Error);
        REQUIRE(diag.kind == InvalidFunctionDefinition.kind);
        REQUIRE(diag.message.format().to_borrowed() == "Invalid function definition for Test at 0");

        {
            REQUIRE(diag.location.filename == simple_converter->filename);
            auto [line_number, column_number] = diag.location.line_info();
            REQUIRE(line_number == 1);
            REQUIRE(column_number == 2);

        }

        {
            auto& context = diag.annotations;
            REQUIRE(context.size() == 5);

            // 1. Insert
            REQUIRE(context[0].level == DiagnosticLevel::Insert);
            REQUIRE(context[0].tokens.lines[0].tokens[0].text.to_borrowed() == ")");
            REQUIRE(context[0].spans.size() == 1);
            REQUIRE(context[0].spans[0] == Span(2, 3));

            // 2. Delete
            REQUIRE(context[1].level == DiagnosticLevel::Delete);
            REQUIRE(context[1].spans.size() == 1);
            REQUIRE(context[1].spans[0] == Span(3, 4));

            // 3. Error Marker
            REQUIRE(context[2].level == DiagnosticLevel::Error);
            REQUIRE(context[2].message.strings[0].first.to_borrowed() == "prototype does not match the defination");
            REQUIRE(context[2].spans.size() == 2);
            REQUIRE(context[2].spans[0] == Span(0, 2));
            REQUIRE(context[2].spans[1] == Span(20, 21));

            // 4. Warning Marker
            REQUIRE(context[3].level == DiagnosticLevel::Warning);
            REQUIRE(context[3].message.empty());
            REQUIRE(context[3].spans.size() == 2);
            REQUIRE(context[3].spans[0] == Span(5, 10));
            REQUIRE(context[3].spans[1] == Span(25, 27));

            // 5. Warning Marker
            REQUIRE(context[4].level == DiagnosticLevel::Note);
            REQUIRE(context[4].message.strings[0].first.to_borrowed() == "Try to fix the error");
            REQUIRE(context[4].spans.size() == 0);
        }
    }
};

TEST_CASE("Simple Diagnostic Builder Output", "[simple_diagnostic:single_line:output]") {
    auto simple_converter = std::make_unique<SimpleConverter>(
        "void test( int a, int c );",
        "main.cpp"
    );
    auto mock = Mock(simple_converter.get());

    {
        mock.clear();

        constexpr auto InvalidFunctionDefinition = dark_make_diagnostic(
            DiagnosticKind::InvalidFunctionDefinition,
            "Invalid function definition for {} at {}",
            char const*, std::uint32_t
        );

        constexpr auto InvalidFunctionPrototype = dark_make_diagnostic(
            DiagnosticKind::InvalidFunctionPrototype,
            "The prototype is defined here"
        );

        auto& emitter = mock.emitter;
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

        auto& consumer = mock.consumer;
        REQUIRE(consumer.diagnostics.size() == 1);
        mock.render_diagnostic();

        auto iter = mock.line_iter();
        REQUIRE(!iter.empty());
        REQUIRE(iter.next() == "Error[E0001]: Invalid function definition for Test at 0");
        REQUIRE(iter.next() == "     ╭─[main.cpp:1:1]");
        REQUIRE(iter.next() == "     │");
        REQUIRE(iter.next() == "   1 |  vo)id test( int a, int c );");
        REQUIRE(iter.next() == "     ┆  ^^+^ xxxx           ~~~~~ ~");
        REQUIRE(iter.next() == "     ┆  ▲      ~~~~         ▲");
        REQUIRE(iter.next() == "     ┆  ├───────────────────╯");
        REQUIRE(iter.next() == "     ┆  │");
        REQUIRE(iter.next() == "     │  │");
        REQUIRE(iter.next() == "     │  │");
        REQUIRE(iter.next() == "     │  │         ╭─ Error ────────────────────────╮");
        REQUIRE(iter.next() == "     │  ╰─────────┤ prototype does not match the   │");
        REQUIRE(iter.next() == "     │            │ defination                     │");
        REQUIRE(iter.next() == "     │            ╰────────────────────────────────╯");
        REQUIRE(iter.next() == "     │");
        REQUIRE(iter.next() == "     │ ╭─ Note ──────────────────╮");
        REQUIRE(iter.next() == "     ╰─┤  Try to fix the error   │");
        REQUIRE(iter.next() == "       ╰─────────────────────────╯");
        REQUIRE(iter.empty());
    }

    {
        mock.clear();
        simple_converter->source =
R"(void test( int a,
           int b,
           int c
        ))";

        constexpr auto InvalidFunctionDefinition = dark_make_diagnostic(
            DiagnosticKind::InvalidFunctionDefinition,
            "Invalid function definition for {} at {}",
            char const*, std::uint32_t
        );

        constexpr auto InvalidFunctionPrototype = dark_make_diagnostic(
            DiagnosticKind::InvalidFunctionPrototype,
            "The prototype is defined here"
        );


        auto& emitter = mock.emitter;
        emitter
            .error(Span(0, 4), InvalidFunctionDefinition, "Test", 0u)
                .begin_annotation()
                     .insert("]", 3)
                     .insert("))))", 1, "Missing parens.")
                     .remove("Remove this thing", Span::from_size(25, 4))
                    .error(
                        "prototype does not match the defination Lorem ipsum dolor sit amet, consectetur adipisicing elit. Eligendi non quis exercitationem culpa nesciunt nihil aut nostrum explicabo reprehenderit optio amet ab temporibus asperiores quasi cupiditate. Voluptatum ducimus voluptates voluptas?",
                        Span::from_size(11, 5),
                        Span::from_size(28, 5),
                        Span::from_size(45, 5)
                    )
                    .warn(Span::from_size(13, 4))
                    .warn("Testing warning.", Span::from_size(11, 5))
                    // .warn(dark::Span(5, 6), dark::Span(6, 8))
                    .note("Try to fix the error")
            .end_annotation()
            .emit();
        auto& consumer = mock.consumer;
        REQUIRE(consumer.diagnostics.size() == 1);
        mock.render_diagnostic();

        auto iter = mock.line_iter();

        REQUIRE(!iter.empty());
        REQUIRE(iter.next() == "Error[E0001]: Invalid function definition for Test at 0");
        REQUIRE(iter.next() == "     ╭─[main.cpp:1:1]");
        REQUIRE(iter.next() == "     │");
        REQUIRE(iter.next() == "   1 |  v))))oi]d test( int a,");
        REQUIRE(iter.next() == "     ┆  ^++++^^+^       ~~~~~");
        REQUIRE(iter.next() == "     ┆   ▲              ~~≈≈≈~");
        REQUIRE(iter.next() == "     ┆   │              ▲");
        REQUIRE(iter.next() == "     ┆   │     ╭────────╯─────────╮");
        REQUIRE(iter.next() == "   2 |   │     │   int b,         │");
        REQUIRE(iter.next() == "     ┆   │     │xxxx              │");
        REQUIRE(iter.next() == "     ┆   │     │▲  ~~~~~          │");
        REQUIRE(iter.next() == "     ┆   │    ╭─╯  ▲              │");
        REQUIRE(iter.next() == "     ┆   │    │├───╯              │");
        REQUIRE(iter.next() == "   3 |   │    ││   int c          │");
        REQUIRE(iter.next() == "     ┆   │    ││   ~~~~~          │");
        REQUIRE(iter.next() == "     ┆   │    ││   ▲              │");
        REQUIRE(iter.next() == "     ┆   │    │├───╯              │");
        REQUIRE(iter.next() == "   4 |   │    ││)                 │");
        REQUIRE(iter.next() == "     │   │    ││                  │");
        REQUIRE(iter.next() == "     │   │    ││                  │");
        REQUIRE(iter.next() == "     │   │    ││        ╭─ Warning ─────────╮");
        REQUIRE(iter.next() == "     │   │    ││        │ Testing warning.  │");
        REQUIRE(iter.next() == "     │   │    ││        ╰───────────────────╯");
        REQUIRE(iter.next() == "     │   │    ││");
        REQUIRE(iter.next() == "     │   │    ││    ╭─ Error ────────────────────────────╮");
        REQUIRE(iter.next() == "     │   │    ││    │ prototype does not match the       │");
        REQUIRE(iter.next() == "     │   │    ││    │ defination Lorem ipsum dolor sit   │");
        REQUIRE(iter.next() == "     │   │    ││    │ amet, consectetur adipisicing      │");
        REQUIRE(iter.next() == "     │   │    ││    │ elit. Eligendi non quis            │");
        REQUIRE(iter.next() == "     │   │    │╰────┤ exercitationem culpa nesciunt      │");
        REQUIRE(iter.next() == "     │   │    │     │ nihil aut nostrum explicabo        │");
        REQUIRE(iter.next() == "     │   │    │     │ reprehenderit optio amet ab        │");
        REQUIRE(iter.next() == "     │   │    │     │ temporibus asperiores quasi        │");
        REQUIRE(iter.next() == "     │   │    │     │ cupiditate. Voluptatum ducimus     │");
        REQUIRE(iter.next() == "     │   │    │     │ voluptates voluptas?               │");
        REQUIRE(iter.next() == "     │   │    │     ╰────────────────────────────────────╯");
        REQUIRE(iter.next() == "     │   │    │");
        REQUIRE(iter.next() == "     │   │    │ ╭─ Delete ───────────╮");
        REQUIRE(iter.next() == "     │   │    ╰─┤ Remove this thing  │");
        REQUIRE(iter.next() == "     │   │      ╰────────────────────╯");
        REQUIRE(iter.next() == "     │   │");
        REQUIRE(iter.next() == "     │   │─ Insert ─────────╮");
        REQUIRE(iter.next() == "     │   │ Missing parens.  │");
        REQUIRE(iter.next() == "     │   ╰──────────────────╯");
        REQUIRE(iter.next() == "     │");
        REQUIRE(iter.next() == "     │ ╭─ Note ──────────────────╮");
        REQUIRE(iter.next() == "     ╰─┤  Try to fix the error   │");
        REQUIRE(iter.next() == "       ╰─────────────────────────╯");
        REQUIRE(iter.empty());
    }
}

TEST_CASE("Tokenized Diagnostic Builder Output", "[tokenized_diagnostic:output]") {
    {
        auto converter = std::make_unique<TokenConverter>(
            "void test( int a, int c );",
            "main.cpp"
        );
        auto mock = Mock(converter.get());
        std::vector<std::string_view> tokens {
            "void",
            " ",
            "test",
            "(",
            " ",
            "int",
            " ",
            "a",
            ",",
            " ",
            "int",
            " ",
            "c",
            " ",
            ")",
            ";"
        };

        {
            auto const& ts = converter->tokens;
            REQUIRE(ts.size() == tokens.size());
            for (auto t = 0ul; t < ts.size(); ++t) {
                auto lhs = ts[t].text(converter->source);
                auto rhs = tokens[t];
                INFO(std::format("[{}]: {} == {}", t, lhs, rhs));
                REQUIRE(lhs == rhs);
            }
        }

        constexpr auto InvalidFunctionPrototype = dark_make_diagnostic(
            DiagnosticKind::InvalidFunctionPrototype,
            "The prototype is defined here"
        );

        auto& emitter = mock.emitter;
        emitter
            .warn(Span(0, 3), InvalidFunctionPrototype)
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

        auto& consumer = mock.consumer;
        REQUIRE(consumer.diagnostics.size() == 1);
        mock.render_diagnostic();

        auto const& diag = consumer.diagnostics[0];

        auto const& source = diag.location.source;

        REQUIRE(diag.level == DiagnosticLevel::Warning);
        REQUIRE(diag.kind == DiagnosticKind::InvalidFunctionPrototype);
        REQUIRE(diag.message.format().to_borrowed() == "The prototype is defined here");
        REQUIRE(diag.location.line_info() == std::make_pair(1, 1));
        REQUIRE(diag.annotations.size() == 5);

        REQUIRE(source.lines.size() == 1);
        REQUIRE(source.lines[0].tokens.size() == tokens.size());

        auto iter = mock.line_iter();

        REQUIRE(!iter.empty());
        REQUIRE(iter.next() == "Warning[W0002]: The prototype is defined here");
        REQUIRE(iter.next() == "     ╭─[main.cpp:1:1]");
        REQUIRE(iter.next() == "     │");
        REQUIRE(iter.next() == "   1 |  vo)id test( int a, int c );");
        REQUIRE(iter.next() == "     ┆  ^^+^ xxxx ~         ~~~~~ ~");
        REQUIRE(iter.next() == "     ┆  ▲      ~~~          ▲");
        REQUIRE(iter.next() == "     ┆  ├───────────────────╯");
        REQUIRE(iter.next() == "     ┆  │");
        REQUIRE(iter.next() == "     │  │");
        REQUIRE(iter.next() == "     │  │");
        REQUIRE(iter.next() == "     │  │         ╭─ Error ────────────────────────╮");
        REQUIRE(iter.next() == "     │  ╰─────────┤ prototype does not match the   │");
        REQUIRE(iter.next() == "     │            │ defination                     │");
        REQUIRE(iter.next() == "     │            ╰────────────────────────────────╯");
        REQUIRE(iter.next() == "     │");
        REQUIRE(iter.next() == "     │ ╭─ Note ──────────────────╮");
        REQUIRE(iter.next() == "     ╰─┤  Try to fix the error   │");
        REQUIRE(iter.next() == "       ╰─────────────────────────╯");
        REQUIRE(iter.empty());
    }
}
