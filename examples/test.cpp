#include <cassert>
#include <format>
#include <print>
#include <cstdlib>

#include "diagnostics.hpp"
#include "diagnostics/core/term/annotated_string.hpp"
#include "diagnostics/core/term/style.hpp"

struct foo {
    int v {};
    foo()
        : v(rand() % 100)
    {
        std::println("foo({})", v);
    }
    foo(foo const& other): v(other.v) {
        std::println("foo(foo const&)[{}]", v);
    }
    foo(foo && other): v(other.v) {
        std::println("foo(foo &&)[{}]", v);
    }
    foo& operator=(foo const& other) {
        v = other.v;
        std::println("foo& operator=(foo const&)[{}]", v);
        return *this;
    }
    foo& operator=(foo && other) {
        v = other.v;
        std::println("foo& operator=(foo &&)[{}]", v);
        return *this;
    }
    ~foo() {
        std::println("~foo({})", v);
    }

    constexpr auto operator==(foo const& other) const noexcept -> bool {
        return v == other.v;
    }

    auto to_string() const -> std::string {
        return std::format("Foo({})", v);
    }
};

/*template <>*/
/*struct std::formatter<foo> {*/
/*    // Parse format specifiers (e.g., ":>20") — forwards to std::string formatter*/
/*    constexpr auto parse(auto& ctx) {*/
/*        auto it = ctx.begin();*/
/*        while (it != ctx.end()) {*/
/*            if (*it == '}') break;*/
/*            ++it;*/
/*        }*/
/*        return it;*/
/*    }*/
/**/
/*    auto format(const foo& f, auto& ctx) const {*/
/*        return std::format_to(ctx.out(), "{}", f.v);*/
/*    }*/
/*};*/

using namespace dark;

std::string_view source = "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.";

struct TestConverter: DiagnosticConverter<unsigned> {
    auto convert_loc(unsigned loc, builder_t&) const -> DiagnosticLocation override {
        std::println("Size: {}", source.size());
        return DiagnosticLocation::from_text(
            "test.cpp",
            // source,
R"(void test( int a,
           int b,
           int c
        ))",
            /*line_number=*/1,
            /*line_start_offset=*/loc,
            /*token_start_offset=*/loc,
            // /*marker=*/Span::from_size(static_cast<dsize_t>(source.size() / 2), 5)
            /*marker=*/Span::from_size(0, 5)
        );
    }
};

int main() {
    auto base = dark_make_diagnostic(0, "Attempted to access index {} of vector with length {}.", int, int);
    auto consumer = ConsoleDiagnosticConsumer();
    auto converter = TestConverter();
    auto emitter = DiagnosticEmitter(
        &converter,
        consumer
    );
    emitter.error(0, base, 5, 3)
        .begin_annotation()
            // .error("unknown method found")
            // .note("Test", Span::from_size(150, 5))
            // .warn("Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.", Span::from_size(150, 5))
            // .note("Warning", Span::from_size(150, 5))
            // .error("Warning", Span::from_size(150, 5))
            // .note(AnnotatedString::builder()
            //         .push("void", { .bold = true })
            //         .push("main", { .text_color = Color::Blue, .padding = term::PaddingValues(0, 1, 0, 1) })
            //         .push("(", { .text_color = Color::Magenta })
            //         .push(")", { .text_color = Color::Magenta })
            //         .build(),
            //       Span::from_size(300, 5)
            //     )
            // .insert("int", 540 + 6, "Try inserting this")
            // .remove("Awesome removal", Span::from_size(540 + 5, 3))
            // .note("invalid use of 'use'")
            // .warn("unknown method found")
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
        .end_annotation()
        .emit();
    return 0;
}

int main2() {

    auto s = dark::Terminal(stdout, TerminalColorMode::Enable);
    auto canvas = term::Canvas(60);
    /*canvas.add_rows(12);*/ 
    /*canvas.draw_line(12, 8, 2, 2);*/

    [[maybe_unused]] auto [bbox, left] = canvas.draw_boxed_text_with_header(
        "Lorem Ipsum",
        AnnotatedString::builder()
            .push("Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.")
            .with_style({
                .text_color = Color::Green,
                .bold = true,
                .padding = term::PaddingValues::all(2),
            })
                .push(" Green")
                .push(" Text")
            .build(),
        0, 0,
        term::TextStyle{
            .text_color = Color::Red,
            .bg_color = Color(39, 39, 39),
            .word_wrap = true,
            // .break_whitespace = true,
            .max_lines = 2,
            .padding = term::PaddingValues::all(2),
            .align = term::TextAlign::Center,
            .overflow = term::TextOverflow::middle_ellipsis,
            .trim_space = true,
        },
        term::TextStyle::default_header_style(),
        term::char_set::box::dotted
    );

    std::println("Left: {}", left);

/*    canvas.draw_marked_text(*/
/*        "void add()",*/
/*        "~",*/
/*0,*/
/*        bbox.bottom_left().second + 2,*/
/*        {},*/
/*        Terminal::Color::RED*/
/*    );*/

    std::vector<term::Point> ps = {
        /*{ 0, 0 },*/
        /*{ 0, 2 },*/
        /*{ 2, 2 }*/
        /*{2,2}, {12,2}, {12,8}, {2,8}, {2,2}*/
    };

    /*for (auto x = 2u; x < 10u; ++x) ps.emplace_back(x, 0);*/
    /*for (auto y = 0u; y < 10u; ++y) ps.emplace_back(10, y);*/
    /*for (auto x = 2u; x < 10u + 1; ++x) ps.emplace_back(10u - x + 2, 10);*/
    /*for (auto y = 0u; y < 10u; ++y) ps.emplace_back(2, 10 - y - 1);*/
    /*for (auto p: ps) {*/
    /*    std::println("HERE: {}, {}", p.x, p.y);*/
    /*}*/

    canvas.draw_path(ps);

    canvas.render(s);

    std::println("Arrow: {}", term::char_set::arrow::basic_bold.right);

    return 0;
}
