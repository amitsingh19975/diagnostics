#ifndef DARK_DIAGNOSTICS_BASIC_HPP
#define DARK_DIAGNOSTICS_BASIC_HPP

#include "diagnostics/core/cow_string.hpp"
#include "diagnostics/core/formatter.hpp"
#include "diagnostics/core/small_vector.hpp"
#include "diagnostics/core/stream.hpp"
#include "diagnostics/span.hpp"
#include <numeric>
#include <string_view>
#include <type_traits>
namespace dark {

    enum class DiagnosticLevel: std::uint8_t {
        Remark = 0,
        Note,
        Warning,
        Error
    };
    

    [[nodiscard]] constexpr auto diagnostic_level_to_color(DiagnosticLevel level) noexcept -> Stream::Color {
        switch (level) {
            case DiagnosticLevel::Error: return Stream::RED;
            case DiagnosticLevel::Warning: return Stream::YELLOW;
            case DiagnosticLevel::Note: return Stream::BLUE;
            case DiagnosticLevel::Remark: return Stream::GREEN;
        }
    }

    [[nodiscard]] constexpr auto diagnostic_level_to_string(DiagnosticLevel level) noexcept -> std::string_view {
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
        Remove,
    };

    static constexpr auto to_string(DiagnosticOperationKind kind) noexcept -> std::string_view {
        switch (kind) {
            case DiagnosticOperationKind::None: return "None";
            case DiagnosticOperationKind::Insert: return "Insert";
            case DiagnosticOperationKind::Remove: return "Remove";
        }
    }

    struct BasicDiagnosticLocationItem {
        std::string_view source{};
        unsigned line_number{};     // 1-based indices where 0 is invalid.
        unsigned column_number{};   // 1-based indices where 0 is invalid.
        unsigned source_location{};
        unsigned length{1};

        [[nodiscard]] constexpr auto col() const noexcept { return (column_number == 0 ? 0 : column_number - 1); }
        [[nodiscard]] constexpr auto span() const noexcept { return Span::from_size(source_location + col(), length); }
    };

    struct BasicDiagnosticTokenItem {
        core::CowString token{};
        unsigned column_number{};  // 1-based indices where 0 is invalid.
        std::optional<Stream::Color> color{};
        bool is_bold{false};

        [[nodiscard]] constexpr auto col() const noexcept { return (column_number == 0 ? 0 : column_number - 1); }
        [[nodiscard]] constexpr auto span() const noexcept -> LocRelSpan {
            return LocRelSpan::from_usize(col(), token.size());
        }
    };

    struct LineDiagnosticToken {
        core::SmallVec<BasicDiagnosticTokenItem> tokens;
        unsigned line_number{};  // 1-based indices where 0 is invalid.
        unsigned source_location{};

        [[nodiscard]] constexpr auto span() const noexcept -> Span {
            if (tokens.empty()) return Span();
            auto res = Span();
            for (auto i = 0zu; i < tokens.size(); ++i) {
                res = res.force_merge(span_for(i));
            }
            return res;
        }
        
        [[nodiscard]] constexpr auto span_for(std::size_t index) const noexcept -> Span {
            return tokens[index].span().resolve(source_location);
        }
    };

    struct DiagnosticLocationTokens {
        core::SmallVec<LineDiagnosticToken> lines;
        Span marker{};

        [[nodiscard]] constexpr auto span() const noexcept -> Span {
            if (lines.empty()) return Span();
            auto temp = std::accumulate(lines.begin(), lines.end(), Span(), [](auto a, auto b) {
               return a.force_merge(b.span());
            });
            return temp;
        }

        [[nodiscard]] constexpr auto get_absolute_position() const noexcept -> unsigned {
            return std::accumulate(lines.begin(), lines.end(), marker.start(), [](auto res, auto const& line) {
                if (line.tokens.empty()) return res;
                return std::min(res, line.source_location);
            });
        }
    };

    namespace detail {
        template <typename T>
        concept is_basic_diagnotic_location_item = std::same_as<BasicDiagnosticLocationItem, std::decay_t<std::remove_cvref_t<T>>>;

        template <typename T>
        concept is_diagnotic_location_tokens = std::same_as<DiagnosticLocationTokens, std::decay_t<std::remove_cvref_t<T>>>;
    } // namespace detail

    struct DiagnosticLocation {
        std::string_view filename{};
        std::variant<BasicDiagnosticLocationItem, DiagnosticLocationTokens> source;

        constexpr auto get_as_location_item() -> BasicDiagnosticLocationItem& {
            return std::get<0>(source);
        }

        constexpr auto get_as_location_item() const -> BasicDiagnosticLocationItem const& {
            return std::get<0>(source);
        }

        constexpr auto get_as_location_tokens() -> DiagnosticLocationTokens& {
            return std::get<1>(source);
        }

        constexpr auto get_as_location_tokens() const -> DiagnosticLocationTokens const& {
            return std::get<1>(source);
        }

        [[nodiscard]] constexpr auto is_location_item() const noexcept -> bool {
            return source.index() == 0;
        }

        [[nodiscard]] constexpr auto is_location_tokens() const noexcept -> bool {
            return !is_location_item();
        }

        [[nodiscard]] constexpr auto get_line_info() const noexcept -> std::pair<unsigned, unsigned> {
            if (is_location_item()) {
                auto& item = get_as_location_item();
                return { item.line_number, item.column_number };
            } else {
                auto& item = get_as_location_tokens();
                auto marker = item.marker;
                for (auto const& line: item.lines) {
                    for (auto const& token: line.tokens) {
                        auto token_span = token.span().resolve(line.source_location);
                        if (token_span.intersects(marker)) {
                            return { line.line_number, std::max(token_span.start(), marker.start()) - line.source_location + 1 };
                        }
                    }
                }
            }
            return { 0, 0 };
        }

        // Used inside the sorted consumer
        constexpr auto operator<(DiagnosticLocation const& other) const noexcept -> bool {
            std::tuple<std::string_view, unsigned, unsigned> lhs{ filename, 0, 0 };
            std::tuple<std::string_view, unsigned, unsigned> rhs{ other.filename, 0, 0 };

            if (is_location_item()) {
                auto const& loc = get_as_location_item();
                std::get<1>(lhs) = loc.line_number;
                std::get<2>(lhs) = loc.column_number;
            } else {
                auto const& loc = get_as_location_tokens();
                if (loc.lines.empty()) return false;
                std::get<1>(lhs) = loc.lines[0].line_number;
                std::get<2>(lhs) = loc.marker.start();
            }

            if (other.is_location_item()) {
                auto const& loc = other.get_as_location_item();
                std::get<1>(rhs) = loc.line_number;
                std::get<2>(rhs) = loc.column_number;
            } else {
                auto const& loc = other.get_as_location_tokens();
                if (loc.lines.empty()) return false;
                std::get<1>(rhs) = loc.lines[0].line_number;
                std::get<2>(rhs) = loc.marker.start();
            }
            return lhs < rhs;
        }
        
        constexpr auto get_marker_span() const noexcept -> Span {
            if (is_location_item()) return get_as_location_item().span();
            else return get_as_location_tokens().marker;
        }

        [[nodiscard]] constexpr auto get_absolute_position() const noexcept -> unsigned {
            if (is_location_item()) return get_as_location_item().source_location;
            return get_as_location_tokens().get_absolute_position();
        }
    };

    struct BasicDiagnosticMessage {
        /**
        * 1. If the "Insert"" operation present, then this message is a string to
        *    be inserted at the span.
        * 2. If the "Remove" operation present, then this message is ignored.
        * 3. If there is no operation, then this mesage is printed as a context
        *    message.
        */
        core::CowString message{};
        DiagnosticLevel level{};
        core::SmallVec<Span, 1> spans{}; // Multiple spans point to the same message
        core::CowString text_to_be_inserted{};
        DiagnosticOperationKind op{ DiagnosticOperationKind::None };

        // Info to identify original marker that we got from the source not from context builder
        bool is_marker {false};
    };

    struct EmptyDiagnosticKind {};

    namespace internal {
        template <typename DiagnosticKindType>
        struct DiagnosticMessageKind {
            DiagnosticKindType kind{};
        };

        template <>
        struct DiagnosticMessageKind<EmptyDiagnosticKind> {
            DARK_NO_UNIQUE_ADDRESS EmptyDiagnosticKind kind{};
        };
    } // namespace internal

    template <typename DiagnosticKindType = EmptyDiagnosticKind>
    struct SubDiagnosticMessage: internal::DiagnosticMessageKind<DiagnosticKindType> {
        using base_type = internal::DiagnosticMessageKind<DiagnosticKindType>;
        using base_type::kind;

        DiagnosticLevel level;
        core::Formatter formatter;
        DiagnosticLocation location{};
        core::SmallVec<BasicDiagnosticMessage, 2> context{};

        constexpr SubDiagnosticMessage() noexcept = default;
        constexpr SubDiagnosticMessage(SubDiagnosticMessage const&) noexcept = default;
        constexpr SubDiagnosticMessage& operator=(SubDiagnosticMessage const&) noexcept = default;
        constexpr SubDiagnosticMessage(SubDiagnosticMessage&&) noexcept = default;
        constexpr SubDiagnosticMessage& operator=(SubDiagnosticMessage&&) noexcept = default;
        constexpr ~SubDiagnosticMessage() noexcept = default;

        SubDiagnosticMessage(
            DiagnosticKindType kind,
            DiagnosticLevel level,
            core::Formatter formatter,
            DiagnosticLocation location
        )
            : base_type { .kind = std::move(kind) }
            , level(level)
            , formatter(std::move(formatter))
            , location(std::move(location))
        {}
    };

    template <typename DiagnosticKindType = EmptyDiagnosticKind>
    struct BasicDiagnostic: internal::DiagnosticMessageKind<DiagnosticKindType> {
        using base_type = internal::DiagnosticMessageKind<DiagnosticKindType>;
        using base_type::kind;

        DiagnosticLevel level{};
        core::Formatter formatter{};
        DiagnosticLocation location{};
        core::SmallVec<BasicDiagnosticMessage, 2> context{};
        core::SmallVec<SubDiagnosticMessage<DiagnosticKindType>, 0> sub_diagnostic{};

        constexpr BasicDiagnostic() noexcept = default;
        BasicDiagnostic(BasicDiagnostic const& other) 
            : base_type(other.kind)
            , level(other.level)
            , formatter(other.formatter)
            , location(other.location)
            , context(other.context)
            , sub_diagnostic(other.sub_diagnostic)
        {}
        BasicDiagnostic& operator=(BasicDiagnostic const& other) {
            BasicDiagnostic temp(other);
            swap(*this, temp);
            return *this;
        }
        constexpr BasicDiagnostic(BasicDiagnostic&& other) noexcept
            : base_type(std::move(other.kind))
            , level(std::move(other.level))
            , formatter(std::move(other.formatter))
            , location(std::move(other.location))
            , context(std::move(other.context))
            , sub_diagnostic(std::move(other.sub_diagnostic))
        {}
        constexpr BasicDiagnostic& operator=(BasicDiagnostic&& other) noexcept {
            BasicDiagnostic temp(std::move(other));
            swap(*this, temp);
            return *this;
        }
        constexpr ~BasicDiagnostic() noexcept = default;

        BasicDiagnostic(
            DiagnosticKindType kind,
            DiagnosticLevel level,
            core::Formatter formatter,
            DiagnosticLocation location
        )
            : base_type { .kind = std::move(kind) }
            , level(level)
            , formatter(std::move(formatter))
            , location(std::move(location))
        {}

        BasicDiagnostic(
            DiagnosticLevel level,
            core::Formatter formatter,
            DiagnosticLocation location
        ) requires std::same_as<DiagnosticKindType, EmptyDiagnosticKind>
            : base_type {}
            , level(level)
            , formatter(std::move(formatter))
            , location(std::move(location))
        {}

        friend void swap(BasicDiagnostic& lhs, BasicDiagnostic rhs) {
            using namespace std;
            swap(lhs.level, rhs.level);
            swap(lhs.kind, rhs.kind);
            swap(lhs.formatter, rhs.formatter);
            swap(lhs.location, rhs.location);
            swap(lhs.context, rhs.context);
            swap(lhs.sub_diagnostic, rhs.sub_diagnostic);
        }
    };

    namespace internal {
        template <typename L, typename R>
        struct is_format_arg_compatible
            : std::is_same<std::decay_t<std::remove_cvref_t<L>>, std::decay_t<std::remove_cvref_t<R>>> {};

        template <typename L>
        struct is_format_arg_compatible<L, void>: std::true_type {};

        template <typename L>
        struct is_format_arg_compatible<L, core::CowString>: std::is_convertible<L, core::CowString> {};

        template <typename R>
        struct is_format_arg_compatible<void, R>: std::true_type {};

        template <typename R>
        struct is_format_arg_compatible<core::CowString, R>: std::is_convertible<R, core::CowString> {};

        template <typename DiagnosticKindType, typename... Args>
        struct DiagnosticBase {
            DiagnosticKindType kind;
            std::string_view format;

            template <core::detail::is_valid_formatter_arg... UArgs>
                requires ((sizeof...(Args) == sizeof...(UArgs)) && (is_format_arg_compatible<Args, UArgs>::value && ...))
            [[nodiscard]] auto apply(UArgs&&... args) const -> core::Formatter {
                return core::Formatter(format, std::forward<std::conditional_t<std::is_void_v<Args>, UArgs, Args>>(args)...);
            }
        };

        template <StaticString Str, typename DiagnosticKindType>
        constexpr auto make_diagnostic_base(DiagnosticKindType&& kind) {
            using type = core::internal::extract_format_types_t<Str>;
            auto helper = [kind]<std::size_t... Is>(std::index_sequence<Is...>) {
                return DiagnosticBase<DiagnosticKindType, typename std::tuple_element_t<Is, type>::type...>{
                    .kind = std::move(kind),
                    .format = Str
                };
            };
            return helper(std::make_index_sequence<std::tuple_size_v<type>>());
        }

    } // namespace internal

} // namespace dark

#define dark_make_diagnostic_with_kind(DiagnosticKind, Format) dark::internal::make_diagnostic_base<Format>(DiagnosticKind)
#define dark_make_diagnostic(Format) dark::internal::make_diagnostic_base<Format>(dark::EmptyDiagnosticKind{})

#endif // DARK_DIAGNOSTICS_BASIC_HPP
