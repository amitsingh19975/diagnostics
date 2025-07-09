# Diagnostics
A diagnostic library for printing compiler diagnostics.

# Inspiration 

- The diagnostic architecture is based on [Google's Carbon Language](https://github.com/carbon-language/carbon-lang) diagnostic implementation, but not the exact implementation.
- `CowString` is inspired by Rust's `Cow` implementation.
- The error reporting or rendering on the terminal is closer to the Rust's.

# Basic Concepts

## 1. Span
- `Span` is an absolute position within the source string. This does not accept signed positions. The range is `[0, max(dsize_t)]`. The default type alias for `dsize_t` is `unsigned`, but if you don't wnat it, you can define the macro `DARK_DIAGNOSTICS_SIZE_TYPE` before the header.

For example:
```cpp
#define DARK_DIAGNOSTICS_SIZE_TYPE std::size_t
#include <diagnostics.hpp>

```

## 2. Annotation/Context
This provides more information to the current diagnostic that will be rendered below the marked span.

```
Source: |xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx|
                                ^				^
                                |				|
                                |				This is an error
                                |- This is a second error.
```

## 3. Diagnostic Location
This is a single string source that supports newlines and could be split into multiple lines or skipped if it has too many lines that don't have diagnostic.

## 4. Diagnostic Converter
This is a custom struct or class that inherits from the `DiagnosticConverter<LocT>`. It allows the user to customise or build diagnostic location to the diagnostic builder.

## 5. Consumer
This an object that consumes the diagnostics, which could a consumer that prints the diagnostics on the terminal or sorts the consumers. These consumers can be plugged into each other; such as plugging sort and stream consumers, which will sort first then print it on the terminal.
There are three predefined consumers:
- `StreamDiagnosticConsumer` This outputs the diagnostic to the `FILE*` (`stderr`, `stdout`, or file)
- `ErrorTrackingDiagnosticConsumer` This tracks the error. If it encounters error, the error flag will be turned on.
- `SortingDiagnosticConsumer` This sorts the diagnostics and needs a explicit flush.

## 6. Format String
This uses the `std::format` under the hood so you can use every options that it uses.
```

# How to use
This library is a C++ header only library so it very easy to use. You can clone the repo and add the include path to the `include` folder inside the cloned repo.

1. Clone the repo
```sh
git clone git@github.com:amitsingh19975/diagnostics.git
```
2. Include it in your project
```cmake
# If you're using CMake
include_directories(repo_path/diagnostics/include)
```
```sh
# You could pass -I flag while compiling
cc -Irepo_path/diagnostics/include my_program
```
3. Including the header inside the C++ project
 ```cpp
 #include <diagnostics.hpp>
 int main() {
	 // ....
 }
```

# Usage
## Defining Diagnostic Converter
```cpp
#include <diagnostics.hpp>

using namespace dark;

struct DiagnosticKind {
    static constexpr std::size_t InvalidFunctionDefinition = 1;
    static constexpr std::size_t InvalidFunctionPrototype = 2;
};

struct TestConverter: DiagnosticConverter<unsigned> {
    std::string_view source;
    std::string_view filename;
    auto convert_loc(unsigned loc, builder_t&) const -> DiagnosticLocation override {
        return DiagnosticLocation::from_text(
            filename,
            source,
            /*line_number=*/1,
            /*line_start_offset=*/loc,
            /*token_start_offset=*/loc,
            /*marker=*/Span::from_size(0, 5)
        );
    }
};

```

## Using The Diagnostic Builder
```cpp
int main() {
    auto consumer = ConsoleDiagnosticConsumer();
    auto converter = TokenConverter("void test( int a, int c );", "test.cpp");
    auto emitter = dark::DiagnosticEmitter(
        &converter,
        consumer
    );

    static constexpr auto InvalidFunctionDefinition = dark_make_diagnostic(
        DiagnosticKind::InvalidFunctionDefinition,
        "Invalid function definition for {} at {}",
        char const*, std::uint32_t
    );

    static constexpr auto InvalidFunctionPrototype = dark_make_diagnostic(
        DiagnosticKind::InvalidFunctionPrototype,
        "The prototype is defined here"
    );

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

    emitter
        .error(Span::from_size(5, 2), InvalidFunctionPrototype)
        .emit();

    return 0;
}
```

## Note:
You can see more examples inside the example folder.

# Outputs/Screenshots

## Example 1
![Example 1](assets/example_1.png)

## Example 2
![Example 2](assets/example_2.png)

## Example 3
![Example 3](assets/example_3.png)

## Example 4
![Example 4](assets/example_4.png)

