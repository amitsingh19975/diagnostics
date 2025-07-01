#ifndef AMT_DARK_DIAGNOSTICS_TERM_ANNOTATED_STRING_HPP
#define AMT_DARK_DIAGNOSTICS_TERM_ANNOTATED_STRING_HPP

#include "../small_vec.hpp"
#include "../cow_string.hpp"
#include "../utf8.hpp"
#include "style.hpp"
#include <cctype>
#include <cstdint>
#include <format>
#include "../../forward.hpp"

namespace dark::term {

    struct SpanStyle {
        std::optional<Color> text_color{};
        std::optional<Color> bg_color{};
        std::optional<bool> bold{};
        std::optional<bool> dim{};
        std::optional<bool> strike{};
        std::optional<bool> italic{};
        std::optional<PaddingValues> padding{};
        std::string_view underline_marker{};

        constexpr auto to_style(TextStyle parent_style) const noexcept -> Style {
            return {
                .text_color = text_color.value_or(parent_style.text_color),
                .bg_color = bg_color.value_or(parent_style.bg_color),
                .bold = bold.value_or(parent_style.bold),
                .dim = dim.value_or(parent_style.dim),
                .strike = strike.value_or(parent_style.strike),
                .italic = strike.value_or(parent_style.italic)
            };
        }

        constexpr auto operator==(SpanStyle const&) const noexcept -> bool = default;
    };

    struct AnnotatedString {
        using base_type = core::SmallVec<std::pair<core::CowString, SpanStyle>>;
        base_type strings;

        struct Builder {
            base_type* ref;
            SpanStyle base_style;

            auto push(core::CowString s) -> Builder {
                if (s.empty()) return *this;
                ref->emplace_back(std::move(s), base_style);
                return *this;
            }
        };

        AnnotatedString() noexcept = default;
        AnnotatedString(AnnotatedString const& other)
            : strings(other.strings)
        {}
        AnnotatedString(AnnotatedString && other) noexcept
            : strings(std::move(other.strings))
        {}
        AnnotatedString& operator=(AnnotatedString const& other) {
            if (this == &other) return *this;
            auto tmp = AnnotatedString(other);
            swap(*this, tmp);
            return *this;
        }
        AnnotatedString& operator=(AnnotatedString && other) noexcept {
            if (this == &other) return *this;
            auto tmp = AnnotatedString(std::move(other));
            swap(*this, tmp);
            return *this;
        }
        ~AnnotatedString() noexcept = default;

        auto push(core::CowString s, SpanStyle style = {}) {
            if (s.empty()) return;
            strings.emplace_back(std::move(s), style);
        }

        auto with_style(SpanStyle style) -> Builder {
            return Builder(&strings, style);
        }

        constexpr auto size() const noexcept -> std::size_t {
            return strings.size();
        }

        constexpr auto empty() const noexcept -> bool {
            return size() == 0;
        }

        [[nodiscard("Missing 'build()' call")]] static auto builder() noexcept -> builder::AnnotatedStringBuilder;

        friend auto swap(AnnotatedString& lhs, AnnotatedString& rhs) noexcept -> void {
            using std::swap;
            swap(lhs.strings, rhs.strings);
        }
    };

} // namespace dark::term

namespace dark {
    using term::SpanStyle, term::AnnotatedString;
} // namespace dark

#include "../../builders/annotated_string.hpp"

namespace dark::term {
    inline auto AnnotatedString::builder() noexcept -> builder::AnnotatedStringBuilder {
        return {};
    }
} // namespace dark::term

template <>
struct std::formatter<dark::term::SpanStyle> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::term::SpanStyle const& s, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "SpanStyle(");
        if (s.text_color) std::format_to(out, "text_color={},", *s.text_color);
        if (s.bg_color) std::format_to(out, "bg_color={},", *s.bg_color);
        if (s.bold) std::format_to(out, "bold={},", *s.bold);
        if (s.dim) std::format_to(out, "dim={},", *s.dim);
        if (s.strike) std::format_to(out, "strike={},", *s.strike);
        if (s.italic) std::format_to(out, "italic={},", *s.italic);
        if (s.padding) std::format_to(out, "padding={},", *s.padding);
        if (!s.underline_marker.empty()) std::format_to(out, "underline_marker='{}'", s.underline_marker);
        std::format_to(out, ")");
        return out;
    }
};

template <>
struct std::formatter<dark::term::AnnotatedString> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::term::AnnotatedString const& s, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "AnnotatedString(strings=[");
        for (auto const& [str, span]: s.strings) {
            std::format_to(out, "String(text='{}', style={}),", str.to_borrowed(), span);
        }
        std::format_to(out, "])");
        return out;
    }
};

#endif // AMT_DARK_DIAGNOSTICS_TERM_ANNOTATED_STRING_HPP
