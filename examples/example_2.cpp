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

    auto convert_loc(Info loc, [[maybe_unused]] builder_t builder) const -> dark::DiagnosticLocation override {
        return dark::DiagnosticLocation {
            .filename = "test.cpp",
            .source = dark::BasicDiagnosticLocationItem {
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

    return 0;
}
