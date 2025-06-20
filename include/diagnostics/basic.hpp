#ifndef AMT_DARK_DIAGNOSTICS_BASIC_HPP
#define AMT_DARK_DIAGNOSTICS_BASIC_HPP

#include "core/config.hpp"
#include "core/term/color.hpp"
#include "core/term/annotated_string.hpp"
#include "forward.hpp"
#include "core/cow_string.hpp"
#include "core/format.hpp"
#include "core/format_any.hpp"
#include "core/small_vec.hpp"
#include "span.hpp"
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <format>
#include <tuple>
#include <type_traits>
#include <utility>

namespace dark {

    enum class DiagnosticLevel: std::uint8_t{
        Remark = 0,
        Note,
        Warning,
        Error,
        Insert,
        Delete
    };

    [[nodiscard]] static inline constexpr auto to_string(DiagnosticLevel level) noexcept -> std::string_view {
        switch (level) {
            case DiagnosticLevel::Error: return "Error";
            case DiagnosticLevel::Warning: return "Warning";
            case DiagnosticLevel::Note: return "Note";
            case DiagnosticLevel::Remark: return "Remark";
            case DiagnosticLevel::Insert: return "Insert";
            case DiagnosticLevel::Delete: return "Delete";
        }
    }

    struct DiagnosticTokenInfo {
        // INFO: Do not change the order of the members since `DiagnosticLineTokenBuilder` relies
        // on this order.

        core::CowString text;
        dsize_t token_start_offset{};
        // Absolute marker
        Span marker{};
        Color text_color{ Color::Default };
        Color bg_color{ Color::Default };
        bool bold{false};
        bool italic{false};

        constexpr auto col(dsize_t line_start_offset) const noexcept -> dsize_t {
            return std::max(line_start_offset, token_start_offset) - token_start_offset;
        }

        // Absoulte span; starting from the source text
        constexpr auto span() const noexcept -> Span {
            return Span::from_size(token_start_offset, static_cast<dsize_t>(text.size()));
        }

        // Absoulte span; starting from the source text
        constexpr auto rel_span(dsize_t line_start_offset) const noexcept -> Span {
            return Span::from_size(
                col(line_start_offset),
                span().size()
            );
        }

        constexpr auto empty() const noexcept -> bool {
            return text.empty();
        }
    };

    struct DiagnosticLineTokens {
        core::SmallVec<DiagnosticTokenInfo> tokens;
        dsize_t line_number{}; // 1-based; 0 is invalid
        dsize_t line_start_offset{};

        // Absoulte span; starting from the source text
        constexpr auto span() const noexcept -> Span {
            if (empty()) return {};
            auto span = Span();

            for (auto i = 0ul; i < tokens.size(); ++i) {
                auto token_span = tokens[i].span();
                span = span.force_merge(token_span);
            }

            return span;
        }

        constexpr auto rel_span() const noexcept -> Span {
            if (empty()) return {};
            auto span = Span();

            for (auto i = 0ul; i < tokens.size(); ++i) {
                auto token_span = tokens[i].rel_span(line_start_offset);
                span = span.force_merge(token_span);
            }

            return span;
        }

        constexpr auto span(std::size_t index) const noexcept -> Span {
            assert(index < tokens.size());
            return tokens[index].span();
        }

        constexpr auto marker(std::size_t index) const noexcept -> Span {
            assert(index < tokens.size());
            return tokens[index].marker;
        }

        constexpr auto empty() const noexcept -> bool { return tokens.empty(); }

        constexpr auto has_any_marker() const noexcept -> bool {
            for (auto const& tok: tokens) {
                if (!tok.marker.empty()) return true;
            }
            return false;
        }

        friend void swap(DiagnosticLineTokens& lhs, DiagnosticLineTokens& rhs) {
            using std::swap;
            swap(lhs.tokens, rhs.tokens);
            swap(lhs.line_number, rhs.line_number);
            swap(lhs.line_start_offset, rhs.line_start_offset);
        }
    };

    struct DiagnosticSourceLocationTokens {
        core::SmallVec<DiagnosticLineTokens> lines{};

        constexpr auto span() const noexcept -> Span {
            if (empty()) return {};
            auto span = Span();

            for (auto i = 0ul; i < lines.size(); ++i) {
                auto line_span = lines[i].span();
                span = span.force_merge(line_span);
            }

            return span;
        }

        constexpr auto rel_span() const noexcept -> Span {
            if (empty()) return {};
            auto span = Span();

            for (auto i = 0ul; i < lines.size(); ++i) {
                auto line_span = lines[i].rel_span();
                span = span.force_merge(line_span);
            }

            return span;
        }

        constexpr auto absolute_line_start() const noexcept -> dsize_t {
            if (empty()) return 0;
            return lines[0].line_start_offset;
        }

        constexpr auto empty() const noexcept -> bool { return lines.empty(); }

        constexpr auto marker() const noexcept -> std::optional<Span> {
            for (auto const& line: lines) {
                for (auto const& tok: line.tokens) {
                    if (!tok.marker.empty()) return tok.marker;
                }
            }
            return {};
        }

        friend void swap(DiagnosticSourceLocationTokens& lhs, DiagnosticSourceLocationTokens& rhs) {
            using std::swap;
            swap(lhs.lines, rhs.lines);
        }

        static constexpr auto builder() noexcept -> builder::DiagnosticTokenBuilder;
    };

    struct DiagnosticLocation {
        std::string_view filename{};
        DiagnosticSourceLocationTokens source;

        constexpr auto line_info() const noexcept -> std::pair<dsize_t, dsize_t> {
            for (auto const& line: source.lines) {
                for (auto const& tok: line.tokens) {
                    if (tok.marker.empty()) continue;
                    auto col = tok.col(line.line_start_offset);
                    return { line.line_number, col == 0 ? 0 : (col + 1) };
                }
            }
            return { 0, 0 };
        }

        friend void swap(DiagnosticLocation& lhs, DiagnosticLocation rhs) {
            using std::swap;
            swap(lhs.filename, rhs.filename);
            swap(lhs.source, rhs.source);
        }

        constexpr auto operator<(DiagnosticLocation const& other) const noexcept -> bool {
            auto l_info = line_info();
            auto r_info = other.line_info();
            auto lhs = std::make_tuple(filename, l_info.first, l_info.second);
            auto rhs = std::make_tuple(other.filename, r_info.first, r_info.second);
            return lhs < rhs;
        }

        static auto from_text(
            std::string_view filename,
            std::string_view text,
            dsize_t line_number, 
            dsize_t line_start,
            dsize_t token_start_offset, 
            Span marker = {}
        ) -> DiagnosticLocation;
    };

    namespace detail {

        #ifdef DARK_DIAGNOSTIC_KIND_TYPE
        using diagnostic_kind_t = DARK_DIAGNOSTIC_KIND_TYPE;
        #else
        using diagnostic_kind_t = std::size_t;
        #endif

        #ifdef DARK_DIAGNOSTIC_KIND_DEFAULT_VALUE
        static constexpr diagnostic_kind_t diagnostic_default_kind = DARK_DIAGNOSTIC_KIND_DEFAULT_VALUE;
        #else
        static constexpr diagnostic_kind_t diagnostic_default_kind = diagnostic_kind_t{};
        #endif

        static_assert(
            (std::integral<diagnostic_kind_t> || std::is_enum_v<diagnostic_kind_t>),
            "Diagnostic kind can only be either integral or enum type"
        );


    } // namespace detail

    struct DiagnosticMessage {
        term::AnnotatedString message{};
        // Could be used for insertion and rendering messages for helper text
        DiagnosticSourceLocationTokens tokens{};
        core::SmallVec<Span, 1> spans{};
        DiagnosticLevel level{};
    };

    struct Diagnostic {
        DiagnosticLevel level{};
        detail::diagnostic_kind_t kind{detail::diagnostic_default_kind};
        DiagnosticLocation location{};
        core::BasicFormatter message{};
        core::SmallVec<DiagnosticMessage, 2> annotations{};
    };

    namespace internal {
        template <core::IsFormattable... Args>
        struct DiagnosticBase {
            detail::diagnostic_kind_t kind{ detail::diagnostic_default_kind };
            core::format_string<Args...> fmt;

            [[nodiscard]] auto apply(Args... args) const -> core::BasicFormatter {
                return core::BasicFormatter(fmt, std::forward<Args>(args)...);
            }
        };
    } // namespace internal
} // namespace dark

#include "builders/token.hpp"
#include "builders/diagnostic.hpp"
#include "builders/annotation.hpp"

namespace dark {

    /**
     * @brief Adds a new text. It'll parse newlines if present.
     * @param line_number line number starts with 1 (1-based index).
     * @param line_start_offset offset to the line start from the start of the source text.
     * @param token_start_offset absolute token position from the start of the source text.
     * @param marker Span of the marker
     */
    inline auto DiagnosticLocation::from_text(
        std::string_view filename,
        std::string_view text,
        dsize_t line_number, 
        dsize_t line_start_offset,
        dsize_t token_start_offset, 
        Span marker
    ) -> DiagnosticLocation {
        auto loc = DiagnosticSourceLocationTokens::builder()
            .add_text(
                text,
                line_number,
                line_start_offset,
                token_start_offset,
                marker
            ).build();
        return { filename, std::move(loc) };
    }

    inline constexpr auto DiagnosticSourceLocationTokens::builder() noexcept -> builder::DiagnosticTokenBuilder {
        return builder::DiagnosticTokenBuilder{};
    }
} // namespace dark

#define dark_make_diagnostic(DiagnosticKind, Format, ...) dark::internal::DiagnosticBase<__VA_ARGS__>((DiagnosticKind), (Format))

template <>
struct std::formatter<dark::DiagnosticLevel> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::DiagnosticLevel const& l, auto& ctx) const {
        return std::format_to(ctx.out(), "{}", dark::to_string(l));
    }
};

template <>
struct std::formatter<dark::DiagnosticTokenInfo> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::DiagnosticTokenInfo const& l, auto& ctx) const {
        return std::format_to(ctx.out(), "DiagnosticTokenInfo(text='{}', marker={}, token_start_offset={}, text_color={}, bg_color={}, bold={})", l.text.to_borrowed(), l.marker, l.token_start_offset, l.text_color, l.bg_color, l.bold);
    }
};

template <>
struct std::formatter<dark::DiagnosticLineTokens> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::DiagnosticLineTokens const& l, auto& ctx) const {
        return std::format_to(ctx.out(), "DiagnosticLineTokens(line_number={}, line_start={}, tokens={})", l.line_number, l.line_start_offset, std::span(l.tokens));
    }
};

template <>
struct std::formatter<dark::DiagnosticSourceLocationTokens> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::DiagnosticSourceLocationTokens const& l, auto& ctx) const {
        return std::format_to(ctx.out(), "DiagnosticSourceTokensLocation(lines={})", std::span(l.lines));
    }
};

template <>
struct std::formatter<dark::DiagnosticLocation> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::DiagnosticLocation const& l, auto& ctx) const {
        return std::format_to(ctx.out(), "DiagnosticLocation(filename='{}', source={})", l.filename, l.source);
    }
};

template <>
struct std::formatter<dark::DiagnosticMessage> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::DiagnosticMessage const& m, auto& ctx) const {
        return std::format_to(ctx.out(), "DiagnosticMessage(message='{}', tokens={}, spans={}, level={})", m.message, m.tokens, std::span(m.spans), m.level);
    }
};

template <>
struct std::formatter<dark::Diagnostic> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    template <dark::core::IsFormattable T>
    auto kind_to_string(T kind) const -> std::string {
        return std::format("{}", dark::core::FormatterAnyArg(kind));
    }

    template <typename T>
    auto kind_to_string(T kind) const -> std::string {
        return std::format("{}", static_cast<std::size_t>(kind));
    }

    auto format(dark::Diagnostic const& d, auto& ctx) const {
        std::string kind = kind_to_string(d.kind);
        return std::format_to(ctx.out(), "Diagnostic(level={}, kind={}, location={}, message={}, annotations={})", d.level, kind, d.location, d.message, d.annotations);
    }
};
#endif // AMT_DARK_DIAGNOSTICS_BASIC_HPP
