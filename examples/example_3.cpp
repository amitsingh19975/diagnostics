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

struct SimpleConverter: DiagnosticConverter<unsigned> {
    auto convert_loc(unsigned loc, [[maybe_unused]] builder_t builder) const -> dark::DiagnosticLocation override {
        return dark::DiagnosticLocation {
            .filename = "test.cpp",
            .source = dark::BasicDiagnosticLocationItem {
                .source = "void test( int a, int c );",
                .line_number = 1,
                .column_number = loc + 1,
                .source_location = 0,
                .length = 2
            }
        };
    }
};

int main() {

    auto stream = dark::Stream(stdout, dark::StreamColorMode::Enable);
    /*auto stream = dark::out();*/
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

    return 0;
}
