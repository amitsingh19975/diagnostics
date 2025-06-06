#ifndef AMT_DARK_DIAGNOSTICS_RENDERER_HPP
#define AMT_DARK_DIAGNOSTICS_RENDERER_HPP

#include "core/term/canvas.hpp"
#include "basic.hpp"
#include "core/term/terminal.hpp"
#include "core/format_any.hpp"
#include "core/term/annotated_string.hpp"
#include "core/term/color.hpp"
#include "diagnostics/core/cow_string.hpp"
#include <concepts>

#ifndef DARK_DIAGNOSTIC_KIND_PADDING
    #define DARK_DIAGNOSTIC_KIND_PADDING 4
#endif

static_assert(std::integral<decltype(DARK_DIAGNOSTIC_KIND_PADDING)>);

namespace dark::internal {

    struct DiagnosticRenderConfig {
        term::BoxCharSet box_normal{ term::char_set::box::rounded };
        term::BoxCharSet box_bold { term::char_set::box::rounded_bold };
        term::LineCharSet line_normal { term::char_set::line::rounded };
        term::LineCharSet line_bold { term::char_set::line::rounded_bold };
        term::ArrowCharSet array_normal { term::char_set::arrow::basic };
        term::ArrowCharSet array_bold { term::char_set::arrow::basic_bold };
    };

    template <core::IsFormattable T>
    auto convert_diagnostic_kind_to_string(T kind) -> std::string {
        if constexpr (std::convertible_to<T, std::size_t>) {
            return std::format("{:0{}}", static_cast<std::size_t>(kind), DARK_DIAGNOSTIC_KIND_PADDING);
        } else {
            return std::format("{}", core::FormatterAnyArg(kind));
        }
    }

    template <typename T>
    auto convert_diagnostic_kind_to_string([[maybe_unused]] T kind) -> std::string {
        if constexpr (std::convertible_to<T, std::size_t>) {
            return std::format("{:0{}}", static_cast<std::size_t>(kind), DARK_DIAGNOSTIC_KIND_PADDING);
        } else {
            return {};
        }
    }

    constexpr auto diagnostic_level_to_color(DiagnosticLevel level) noexcept -> Color {
        switch (level) {
        case DiagnosticLevel::Remark: return Color::Green;
        case DiagnosticLevel::Note: return Color::Blue;
        case DiagnosticLevel::Warning: return Color::Yellow;
        case DiagnosticLevel::Error: return Color::Red;
        case DiagnosticLevel::Insert: return Color::Green;
        case DiagnosticLevel::Delete: return Color::Red;
          break;
        }
    }
    constexpr auto diagnostic_level_code_prefix(DiagnosticLevel level) noexcept -> std::string_view {
        switch (level) {
        case DiagnosticLevel::Remark: return "E";
        case DiagnosticLevel::Note: return "N";
        case DiagnosticLevel::Warning: return "W";
        case DiagnosticLevel::Error: return "E";
        case DiagnosticLevel::Insert: return "I";
        case DiagnosticLevel::Delete: return "D";
          break;
        }
    }

    static inline auto make_diagnostic_message(Diagnostic& diag) -> AnnotatedString {
        auto builder = AnnotatedString::builder();
        auto span_style = SpanStyle {
            .text_color = diagnostic_level_to_color(diag.level),
            .bold = true
        };
        auto tmp = builder.with_style(span_style).push(to_string(diag.level));
        auto code = convert_diagnostic_kind_to_string(diag.kind);
        if (!code.empty()) {
            tmp = tmp
                .push("[")
                .push(diagnostic_level_code_prefix(diag.level))
                .push(core::CowString(code))
                .push("]");
        }
        return tmp
            .push(":")
            .with_style({ .bold = true })
            .push(" ")
            .push(diag.message.format())
            .build();
    }

    static inline auto render_diagnostic(
        Terminal& term,
        Diagnostic& diag,
        DiagnosticRenderConfig config = {}
    ) -> void {
        auto canvas = term::Canvas(term.columns());
        (void) config;

        auto message = make_diagnostic_message(diag);
        canvas.draw_text(message, 0, 0);


        canvas.render(term);
    }

} // namespace dark::internal

#endif // AMT_DARK_DIAGNOSTICS_RENDERER_HPP
