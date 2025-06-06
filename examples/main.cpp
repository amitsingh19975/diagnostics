#include <cassert>
#include <format>
#include <numeric>
#include <print>
#include <cstdlib>
#include <sstream>

#include "diagnostics/consumer.hpp"
#include "diagnostics/converter.hpp"
#include "diagnostics/core/config.hpp"
#include "diagnostics/core/cow_string.hpp"
#include "diagnostics/core/format.hpp"
#include "diagnostics/core/small_vec.hpp"
#include "diagnostics/core/term/annotated_string.hpp"
#include "diagnostics/core/term/basic.hpp"
#include "diagnostics/core/term/canvas.hpp"
#include "diagnostics/core/term/terminal.hpp"
#include "diagnostics/core/utf8.hpp"
#include "diagnostics/basic.hpp"
#include "diagnostics/emitter.hpp"
#include "diagnostics/builders/annotation.hpp"

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

struct TestConverter: DiagnosticConverter<unsigned> {
    auto convert_loc(unsigned loc, builder_t&) const -> DiagnosticLocation override {
        return DiagnosticLocation::from_text(
            "test.cpp",
            "void main(int argc, char** argv)\n{\n return 0;\n}\n",
            0,
            loc,
            4,
            4
        );
    }
};

int main2() {
    auto base = dark_make_diagnostic(0, "{}", int);
    auto consumer = ConsoleDiagnosticConsumer();
    auto converter = TestConverter();
    auto emitter = DiagnosticEmitter(
        &converter,
        consumer
    );
    emitter.error(2, base, 4)
        .begin_annotation()
            .error("unknown method found")
        .end_annotation()
        .emit();
    //d.error(base, 4)
    //    .begin_context()
    //        .note("try remove '{}'", '+')
    //        .insert("+")
    //        .insert("+", /*offset from the start of marker*/ 1)
    //        .remove(/*size*/3)
    //        .remove(/*offset*/2, /*size*/3)
    //        .error("invalid use of '+'", Span(...), Span(...), ...)
    //        .begin_error("invalid use of '+'")
    //            .add_span(Span())
    //            .add_abs_span(Span())
    //            .add_line_start_span(Span())
    //        .end_error()
    //    .end_context()
    //    .emit();
    /*std::println("{}", base.apply(4));*/
    return 0;
}

int main() {

    auto s = dark::Terminal(stdout, Terminal::ColorMode::Enable);
    auto canvas = term::Canvas(60);
    /*canvas.add_rows(12);*/
    /*canvas.draw_line(12, 8, 2, 2);*/

    [[maybe_unused]] auto [bbox, left] = canvas.draw_boxed_text(
        AnnotatedString::builder()
            .push("Lorem Ipsum is simply dummy\ntext of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.")
            .with_style({
                .text_color = Terminal::GREEN,
                .bold = true,
                .padding = term::PaddingValues::all(2),
            })
                .push(" Green")
                .push(" Text")
            .build(),
        0, 0,
        term::TextStyle{
            .text_color = Terminal::RED,
            .word_wrap = true,
            .break_whitespace = true,
            .padding = term::PaddingValues::all(2),
            .trim_prefix = true,
        },
        term::char_set::box::doubled
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
