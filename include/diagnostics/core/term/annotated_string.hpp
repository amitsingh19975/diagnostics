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

        constexpr auto size() const noexcept -> std::size_t {
            return m_cached_index.size() - m_offset;
        }

        auto build_indices() const -> void {
            m_offset = 0;
            for (auto i = 0ul; i < strings.size(); ++i) {
                auto const& s = strings[i].first.to_borrowed();
                for (auto j = 0ul, k = 0ul; j < s.size(); ++k) {
                    auto len = core::utf8::get_length(s[j]);
                    m_cached_index.emplace_back(i, j, len, k);
                    j += len;
                }
            }
        }

        constexpr auto operator[](std::size_t k) noexcept -> std::tuple<std::string_view, SpanStyle, std::string_view> {
            auto idx = m_offset + k;
            assert(idx < m_cached_index.size());
            auto [s_idx, c_idx, len, ridx] = m_cached_index[idx];
            auto const& [text, style] = strings[s_idx];
            auto ns = style;
            if (style.padding) {
                if (c_idx != 0) {
                    ns.padding->left = 0;
                    if (c_idx + len < text.size()) {
                        ns.padding->right = 0;
                    }
                } else {
                    ns.padding->right = 0;
                }
            }

            auto marker = std::string_view{};
            if (!style.underline_marker.empty()) {
                auto m_text = core::utf8::PackedUTF8(style.underline_marker);
                auto m_idx = ridx % m_text.size();
                marker = m_text[m_idx];
                ns.padding->bottom += static_cast<unsigned>(!marker.empty());
            }
            return { text.to_borrowed().substr(c_idx, len), ns, marker };
        }

        constexpr auto operator[](std::size_t k) const noexcept -> std::tuple<std::string_view, SpanStyle, std::string_view> {
            auto* self = const_cast<AnnotatedString*>(this); 
            return self->operator[](k);
        }

        constexpr auto shift(std::size_t offset) const noexcept -> void {
            m_offset = std::min(m_cached_index.size(), m_offset + offset);
        }

        constexpr auto empty() const noexcept -> bool {
            return size() == 0;
        }

        constexpr auto update_padding(std::size_t index, PaddingValues p) noexcept {
            auto idx = m_offset + index;
            assert(idx < m_cached_index.size());
            auto [s_idx, c_idx, len, ridx] = m_cached_index[idx];
            (void)ridx;
            auto& style = strings[s_idx].second;
            style.padding = p;
        }

        constexpr auto is_word_end(std::size_t k) const noexcept -> bool {
            auto idx = m_offset + k;
            if (idx >= m_cached_index.size()) return true;
            auto [s_idx, c_idx, len, ridx] = m_cached_index[idx];
            (void)ridx;
            auto const& text = strings[s_idx].first.to_borrowed();
            if (std::isspace(text[c_idx])) return true;
            return c_idx == text.size();
        }

        [[nodiscard("Missing 'build()' call")]] static auto builder() noexcept -> builder::AnnotatedStringBuilder;
    private:
        // (string index, char index, len, relative index)
        mutable core::SmallVec<std::tuple<std::size_t, std::size_t, std::uint8_t, std::size_t>, 64> m_cached_index{};
        mutable std::size_t m_offset{};
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
