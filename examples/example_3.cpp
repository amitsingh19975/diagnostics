#include "diagnostics.hpp"

using namespace dark;

std::string_view source = "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.";

struct TestConverter: DiagnosticConverter<unsigned> {
    auto convert_loc(unsigned loc, builder_t&) const -> DiagnosticLocation override {
        return DiagnosticLocation::from_text(
            "test.cpp",
            source,
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
            .note("Test", Span::from_size(150, 5))
            .warn("Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.", Span::from_size(150, 5))
            .note("Warning", Span::from_size(150, 5))
            .error("Warning", Span::from_size(150, 5))
            .note(AnnotatedString::builder()
                    .push("void", { .bold = true })
                    .push("main", { .text_color = Color::Blue, .padding = term::PaddingValues(0, 1, 0, 1) })
                    .push("(", { .text_color = Color::Magenta })
                    .push(")", { .text_color = Color::Magenta })
                    .build(),
                  Span::from_size(300, 5)
                )
            .insert("int", 540 + 6, "Try inserting this")
            .remove("Awesome removal", Span::from_size(540 + 5, 3))
            .note("invalid use of 'use'")
            .warn("unknown method found")
        .end_annotation()
        .emit();

    return 0;
}
