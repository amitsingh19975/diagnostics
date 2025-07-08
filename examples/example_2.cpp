#include "diagnostics.hpp"

using namespace dark;

std::string_view source = "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.";

struct TestConverter: DiagnosticConverter<unsigned> {
    auto convert_loc(unsigned loc, builder_t&) const -> DiagnosticLocation override {
        return DiagnosticLocation::from_text(
            "test.cpp",
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
