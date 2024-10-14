#include "diagnostics/basic.hpp"
#include "mock.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace dark;

TEST_CASE("Diagnostic Builder", "[diagnostic:builder]") {
    auto mock = Mock<SingleLineConverter>();
    mock.converter.source = "void test( int a, int c );";

    {
        mock.clear();

        constexpr auto InvalidFunctionDefinition = dark_make_diagnostic_with_kind(
            DiagnosticKind::InvalidFunctionDefinition,
            "Invalid function definition for {s} at {u32}"
        );

        constexpr auto InvalidFunctionPrototype = dark_make_diagnostic_with_kind(
            DiagnosticKind::InvalidFunctionPrototype,
            "The prototype is defined here"
        );

        auto& emitter = mock.emitter;
        mock.converter.absolute_position = 1;
        emitter
            .error(1, InvalidFunctionDefinition, "Test", 0u)
            .context(
                dark::DiagnosticContext()
                    .insert(")", 2)
                    .insert_marker_rel(")", 3)
                    .del(dark::MarkerRelSpan(3, 7))
                    .error(
                        "prototype does not match the defination",
                        dark::Span(0, 2),
                        dark::Span(20, 21)
                    )
                    .warn(dark::Span(5, 10), dark::Span(25, 27))
                    .note("Try to fix the error")
            )
            .sub_diagnostic()
                .warn(0, InvalidFunctionPrototype)
            .build()
            .emit();

        auto& consumer = mock.consumer;
        REQUIRE(consumer.diags.size() == 1);
        auto& diag = consumer.diags[0];

        REQUIRE(diag.level == DiagnosticLevel::Error);

        REQUIRE(diag.level == DiagnosticLevel::Error);
        REQUIRE(diag.kind == InvalidFunctionDefinition.kind);
        REQUIRE(diag.formatter.format() == "Invalid function definition for Test at 0");

        {
            REQUIRE(diag.location.filename == mock.converter.filename);
            REQUIRE(diag.location.is_location_item());

            auto& info = diag.location.get_as_location_item();
            REQUIRE(info.line_number == 1);
            REQUIRE(info.column_number == 2);
            REQUIRE(info.source_location == 1);
            REQUIRE(info.source == mock.converter.source);
            REQUIRE(info.span() == Span(2, 3));
        }

        {
            auto& context = diag.context;
            REQUIRE(context.size() == 6);

            // 1. Insert
            REQUIRE(context[0].op == DiagnosticOperationKind::Insert);
            REQUIRE(context[0].text_to_be_inserted == ")");
            REQUIRE(context[0].spans.size() == 1);
            REQUIRE(context[0].spans[0] == Span(2, 3));

            // 2. Insert Relative
            REQUIRE(context[1].op == DiagnosticOperationKind::Insert);
            REQUIRE(context[1].text_to_be_inserted == ")");
            REQUIRE(context[1].spans.size() == 1);
            REQUIRE(context[1].spans[0] == Span(5, 6)); // abs_pos + col + 3 == 1 + 2 + 3 = 5

            // 3. Delete Relative
            REQUIRE(context[2].op == DiagnosticOperationKind::Remove);
            REQUIRE(context[2].spans.size() == 1);
            REQUIRE(context[2].spans[0] == Span(5, 9));

            // 4. Error Marker
            REQUIRE(context[3].level == DiagnosticLevel::Error);
            REQUIRE(context[3].message == "prototype does not match the defination");
            REQUIRE(context[3].spans.size() == 2);
            REQUIRE(context[3].spans[0] == Span(0, 2));
            REQUIRE(context[3].spans[1] == Span(20, 21));

            // 5. Warning Marker
            REQUIRE(context[4].level == DiagnosticLevel::Warning);
            REQUIRE(context[4].message == "");
            REQUIRE(context[4].spans.size() == 2);
            REQUIRE(context[4].spans[0] == Span(5, 10));
            REQUIRE(context[4].spans[1] == Span(25, 27));

            // 6. Warning Marker
            REQUIRE(context[5].level == DiagnosticLevel::Note);
            REQUIRE(context[5].message == "Try to fix the error");
            REQUIRE(context[5].spans.size() == 0);
        }

        auto& sub_diag = diag.sub_diagnostic;
        REQUIRE(sub_diag.size() == 1);
        {
            auto diag = sub_diag[0];
            REQUIRE(diag.context.empty() == true);
            REQUIRE(diag.formatter.format() == "The prototype is defined here");
            REQUIRE(diag.kind == DiagnosticKind::InvalidFunctionPrototype);
            REQUIRE(diag.level == DiagnosticLevel::Warning);
            REQUIRE(diag.location.filename == mock.converter.filename);
            REQUIRE(diag.location.is_location_item());

            auto& info = diag.location.get_as_location_item();
            REQUIRE(info.line_number == 1);
            REQUIRE(info.column_number == 1);
            REQUIRE(info.source_location == 1);
            REQUIRE(info.source == mock.converter.source);
            REQUIRE(info.span() == Span(1, 2));
        }
    }
}

TEST_CASE("Simple Diagnostic Builder Output", "[simple_diagnostic:single_line:output]") {
    {
        auto mock = Mock<SingleLineConverter>(); 
        mock.converter.source = "void test( int a, int c );";

        constexpr auto InvalidFunctionDefinition = dark_make_diagnostic_with_kind(
            DiagnosticKind::InvalidFunctionDefinition,
            "Invalid function definition for {s} at {u32}"
        );

        constexpr auto InvalidFunctionPrototype = dark_make_diagnostic_with_kind(
            DiagnosticKind::InvalidFunctionPrototype,
            "The prototype is defined here"
        );

        auto& emitter = mock.emitter;
        mock.converter.absolute_position = 1;
        emitter
            .error(1, InvalidFunctionDefinition, "Test", 0u)
            .context(
                dark::DiagnosticContext()
                    .insert(")", 2)
                    .insert_marker_rel(")", 3)
                    .del(dark::MarkerRelSpan(4, 8))
                    .error(
                        "prototype does not match the defination",
                        dark::Span(0, 2),
                        dark::Span(19, 24)
                    )
                    .warn(dark::Span(6, 10), dark::Span(25, 27))
                    .note("Try to fix the error")
            )
            .sub_diagnostic()
                .warn(0, InvalidFunctionPrototype)
            .build()
            .emit();

        auto& consumer = mock.consumer;
        REQUIRE(consumer.diags.size() == 1);

        auto iter = mock.as_line_iter();
        REQUIRE(!iter.empty());
        REQUIRE(iter.next() == "error: Invalid function definition for Test at 0");
        REQUIRE(iter.next() == "   --> test.cpp:1:2");
        REQUIRE(iter.next() == "  1 | v)oid t)est( int a, int c );");
        REQUIRE(iter.next() == "    : ^+^   -+---         ^~~~~ ~~");
        REQUIRE(iter.next() == "    : |     ~ ~~~         |");
        REQUIRE(iter.next() == "    : |                   |");
        REQUIRE(iter.next() == "    : |--------------------prototype does not match the defination");
        REQUIRE(iter.next() == "    :");
        REQUIRE(iter.next() == "    = note: Try to fix the error");
        REQUIRE(iter.next() == "warning: The prototype is defined here");
        REQUIRE(iter.next() == "   --> test.cpp:1:1");
        REQUIRE(iter.next() == "  1 | void test( int a, int c );");
        REQUIRE(iter.next() == "    : ^");
        REQUIRE(iter.next() == "    :");
        REQUIRE(iter.next() == "");
        REQUIRE(iter.next() == "");
        REQUIRE(iter.empty());
    }

    {
        auto mock = Mock<MultilineConverter>(); 

        constexpr auto InvalidFunctionDefinition = dark_make_diagnostic_with_kind(
            DiagnosticKind::InvalidFunctionDefinition,
            "Invalid function definition for {s} at {u32}"
        );

        constexpr auto InvalidFunctionPrototype = dark_make_diagnostic_with_kind(
            DiagnosticKind::InvalidFunctionPrototype,
            "The prototype is defined here"
        );

        auto& emitter = mock.emitter;
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
                .error({222, 4}, InvalidFunctionPrototype)
            .build()
            .emit();
        auto& consumer = mock.consumer;
        REQUIRE(consumer.diags.size() == 1);

        auto iter = mock.as_line_iter();

        REQUIRE(!iter.empty());
        REQUIRE(iter.next() == "error: Invalid function definition for Test at 0");
        REQUIRE(iter.next() == "    --> test.cpp:23:2");
        REQUIRE(iter.next() == "  23 | voi]d)))) test( int a,");
        REQUIRE(iter.next() == "     :  ^^+ ++++----   ^~~~~");
        REQUIRE(iter.next() == "     :      |---+------|");
        REQUIRE(iter.next() == "     :      ||  |      ^~~~~~");
        REQUIRE(iter.next() == "     :      ||--+------|");
        REQUIRE(iter.next() == "  24 |      ||| |>int b,");
        REQUIRE(iter.next() == "     :      ||| | ~~~~~");
        REQUIRE(iter.next() == "  25 |      ||| |>int c");
        REQUIRE(iter.next() == "     :      ||| | ~~~~~");
        REQUIRE(iter.next() == "  26 |      |||)|");
        REQUIRE(iter.next() == "     :      ||| |");
        REQUIRE(iter.next() == "     :      ||| |-------prototype does not match the defination Lorem ipsum dolor sit amet,");
        REQUIRE(iter.next() == "     :      |||          consectetur adipisicing elit. Eligendi non quis exercitationem culpa");
        REQUIRE(iter.next() == "     :      |||          nesciunt nihil aut nostrum explicabo reprehenderit optio amet ab");
        REQUIRE(iter.next() == "     :      |||          temporibus asperiores quasi cupiditate. Voluptatum ducimus voluptates");
        REQUIRE(iter.next() == "     :      |||          voluptas?");
        REQUIRE(iter.next() == "     :      |||-------Testing warning.");
        REQUIRE(iter.next() == "     :      ||---Remove this thing");
        REQUIRE(iter.next() == "     :      -Missing parens.");
        REQUIRE(iter.next() == "     :");
        REQUIRE(iter.next() == "     = note: Try to fix the error");
        REQUIRE(iter.next() == "error: The prototype is defined here");
        REQUIRE(iter.next() == "    --> test.cpp:223:5");
        REQUIRE(iter.next() == " 223 | void test( int a,");
        REQUIRE(iter.next() == "     :     ^^");
        REQUIRE(iter.next() == " 224 |            int b,");
        REQUIRE(iter.next() == " 225 |            int c");
        REQUIRE(iter.next() == " 226 |         )");
        REQUIRE(iter.next() == "     :");
        REQUIRE(iter.next() == "");
        REQUIRE(iter.next() == "");
        REQUIRE(iter.empty());
    }
}

TEST_CASE("Tokenized Diagnostic Builder Output", "[tokenized_diagnostic:single_line:output]") {
    {
        auto mock = Mock<TokenConverter>(); 

        constexpr auto InvalidFunctionDefinition = dark_make_diagnostic_with_kind(
            DiagnosticKind::InvalidFunctionDefinition,
            "Invalid function definition for {s} at {u32}"
        );

        constexpr auto InvalidFunctionPrototype = dark_make_diagnostic_with_kind(
            DiagnosticKind::InvalidFunctionPrototype,
            "The prototype is defined here"
        );

        auto& emitter = mock.emitter;
        emitter
            .error({1, 1}, InvalidFunctionDefinition, "Test", 0u)
            .context(
                dark::DiagnosticContext()
                    .insert(")", 2)
                    .insert_marker_rel(")", 3)
                    .del(dark::MarkerRelSpan(4, 8))
                    .error(
                        "prototype does not match the defination",
                        dark::Span(0, 2),
                        dark::Span(19, 24)
                    )
                    .warn(dark::Span(6, 10), dark::Span(25, 27))
                    .note("Try to fix the error")
            )
            .sub_diagnostic()
                .warn({0, 1}, InvalidFunctionPrototype)
            .build()
            .emit();

        auto& consumer = mock.consumer;
        REQUIRE(consumer.diags.size() == 1);

        auto iter = mock.as_line_iter();
        REQUIRE(!iter.empty());
        REQUIRE(iter.next() == "error: Invalid function definition for Test at 0");
        REQUIRE(iter.next() == "   --> test.cpp:2:2");
        REQUIRE(iter.next() == "  2 |  i)nt )test ( int a , int b )");
        REQUIRE(iter.next() == "    : ^^+  -+---           ^~~~~ ~~");
        REQUIRE(iter.next() == "    : |    ~ ~~~~~         |");
        REQUIRE(iter.next() == "    : |                    |");
        REQUIRE(iter.next() == "    : |-------------------------prototype does not match the defination");
        REQUIRE(iter.next() == "    :");
        REQUIRE(iter.next() == "    = note: Try to fix the error");
        REQUIRE(iter.next() == "warning: The prototype is defined here");
        REQUIRE(iter.next() == "   --> test.cpp:1:2");
        REQUIRE(iter.next() == "  1 |  int test ( int a , int b )");
        REQUIRE(iter.next() == "    : ^^");
        REQUIRE(iter.next() == "    :");
        REQUIRE(iter.next() == "");
        REQUIRE(iter.next() == "");
        REQUIRE(iter.empty());
    }
}
