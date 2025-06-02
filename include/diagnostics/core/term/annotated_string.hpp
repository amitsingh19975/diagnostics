#ifndef AMT_DARK_DIAGNOSTICS_TERM_ANNOTATED_STRING_HPP
#define AMT_DARK_DIAGNOSTICS_TERM_ANNOTATED_STRING_HPP

#include "../small_vec.hpp"
#include "../cow_string.hpp"
#include "style.hpp"

namespace dark::term {

    struct SpanStyle {
        std::optional<Terminal::Color> text_color{};
        std::optional<Terminal::Color> bg_color{};
        std::optional<bool> bold{};
        std::optional<bool> dim{};
        std::optional<bool> strike{};
        std::optional<PaddingValues> padding{};

        constexpr auto to_style(TextStyle parent_style) const noexcept -> Style {
            return {
                .text_color = text_color.value_or(parent_style.text_color),
                .bg_color = bg_color.value_or(parent_style.bg_color),
                .bold = bold.value_or(parent_style.bold),
                .dim = dim.value_or(parent_style.dim),
                .strike = strike.value_or(parent_style.strike)
            };
        }
    };

    struct AnnotatedString {
        using base_type = core::SmallVec<std::pair<core::CowString, SpanStyle>>;
        base_type strings;

        struct Builder {
            base_type* ref;
            SpanStyle base_style;

            auto push(core::CowString s) -> Builder {
                ref->emplace_back(std::move(s), base_style);
                return *this;
            }
        };

        auto push(core::CowString s, SpanStyle style = {}) {
            strings.emplace_back(std::move(s), style);
        }

        auto with_style(SpanStyle style) -> Builder {
            return Builder(&strings, style);
        }
    };

} // namespace dark::term

#endif // AMT_DARK_DIAGNOSTICS_TERM_ANNOTATED_STRING_HPP
