 #include "diagnostics.hpp"

template <typename LocT>
using DiagnosticConverter = dark::BasicDiagnosticConverter<LocT>;

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

    auto consumer = dark::BasicStreamDiagnosticConsumer(dark::out());
    auto converter = SimpleConverter();
    auto emitter = dark::BasicDiagnosticEmitter(
        converter,
        consumer
    );

    static constexpr auto InvalidFunctionDefinition = dark_make_diagnostic(
        "Invalid function definition for {s} at {u32}"
    );

    emitter
        .error(1, InvalidFunctionDefinition, "Test", 0u)
        .context(
            dark::DiagnosticContext()
                .error(
                    "prototype does not match the defination",
                    dark::Span::from_size(5, 4)
                )
                .error(
                    "Unknown error occured",
                    dark::Span::from_size(5, 4)
                )
                .error(
                    "Unknown error occured",
                    dark::Span::from_size(5, 4)
                )
        )
        .emit();

    return 0;
}
