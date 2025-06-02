#ifndef AMT_DARK_DIAGNOSTICS_BASIC_HPP
#define AMT_DARK_DIAGNOSTICS_BASIC_HPP

#include "core/config.hpp"
#include "diagnostics/core/small_vec.hpp"
#include "span.hpp"
#include <algorithm>
#include <cstdint>
#include <format>

namespace dark {

    enum class DiagnosticLevel: std::uint8_t{
        Remark = 0,
        Note,
        Warning,
        Error
    };

    [[nodiscard]] static inline constexpr auto to_string(DiagnosticLevel level) noexcept -> std::string_view {
        switch (level) {
            case DiagnosticLevel::Error: return "error";
            case DiagnosticLevel::Warning: return "warning";
            case DiagnosticLevel::Note: return "note";
            case DiagnosticLevel::Remark: return "remark";
        }
    }

    enum class DiagnosticOperationKind: std::uint8_t {
        None,
        Insert,
        Delete,
    };

    [[nodiscard]] static inline constexpr auto to_string(DiagnosticOperationKind op) noexcept -> std::string_view {
        switch (op) {
            case DiagnosticOperationKind::None: return "none";
            case DiagnosticOperationKind::Insert: return "insert";
            case DiagnosticOperationKind::Delete: return "delete";
        }
    }

    enum class TokenColor {
        None = 0,
        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
    };

    struct DiagnosticSourceLocation {
        std::string_view text;
        dsize_t len{};
        dsize_t line_number{}; // 1-based; 0 is invalid
        dsize_t column_number{}; // 1-based; 0 is invalid
        dsize_t absolute_line_start_location{}; // position from the start of the source

        constexpr auto col() const noexcept -> dsize_t {
            return std::max(column_number, dsize_t{1}) - 1;
        }

        // Absoulte span; starting from the source text
        constexpr auto span() const noexcept -> Span {
            return Span::from_size(absolute_line_start_location + col(), len);
        }

        constexpr auto empty() const noexcept -> bool {
            return len == 0;
        }
    };

    struct DiagnosticTokenInfo {
        std::string_view token;
        dsize_t len{};
        dsize_t column_number{}; // 1-based; 0 is invalid
        TokenColor color{ TokenColor::None };
        bool bold{false};

        constexpr auto col() const noexcept -> dsize_t {
            return std::max(column_number, dsize_t{1}) - 1;
        }

        // Absoulte span; starting from the source text
        constexpr auto span(dsize_t line_start) const noexcept -> Span {
            return Span::from_size(line_start + col(), len);
        }

        constexpr auto empty() const noexcept -> bool {
            return len == 0;
        }
    };

    struct DiagnosticLineTokens {
        core::SmallVec<DiagnosticTokenInfo> tokens;
        dsize_t line_number{}; // 1-based; 0 is invalid
        dsize_t absolute_line_start_location{}; // position from the start of the source

        constexpr auto span() const noexcept -> Span {
            if (empty()) return {};
            auto span = Span();

            for (auto i = 0ul; i < tokens.size(); ++i) {
                auto token_span = tokens[i].span(absolute_line_start_location);
                span = span.force_merge(token_span);
            }

            return span;
        }

        constexpr auto span(std::size_t index) const noexcept -> Span {
            assert(index < tokens.size());
            return tokens[index].span(absolute_line_start_location);
        }

        constexpr auto empty() const noexcept -> bool { return tokens.empty(); }
    };
} // namespace dark


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
struct std::formatter<dark::DiagnosticOperationKind> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(dark::DiagnosticOperationKind const& o, auto& ctx) const {
        return std::format_to(ctx.out(), "{}", dark::to_string(o));
    }
};

#endif // AMT_DARK_DIAGNOSTICS_BASIC_HPP
