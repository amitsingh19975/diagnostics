#ifndef AMT_DARK_DIAGNOSTICS_FORWARD_HPP
#define AMT_DARK_DIAGNOSTICS_FORWARD_HPP

#include <cstdint>
namespace dark {

    enum class DiagnosticLevel: std::uint8_t;
    enum class DiagnosticOperationKind: std::uint8_t;

    struct DiagnosticTokenInfo;
    struct DiagnosticLineTokens;
    struct DiagnosticSourceLocationTokens;
    struct DiagnosticLocation;
    struct DiagnosticMessage;
    struct Diagnostic;

    template <typename LocT>
    struct DiagnosticConverter;

    template <typename LocT, typename AnnotateFn>
    struct DiagnosticAnnotationScope;

    template <typename LocT>
    struct DiagnosticEmitter;

    struct Span;

    struct DiagnosticConsumer;
    struct ErrorTrackingDiagnosticConsumer;
    struct SortingDiagnosticConsumer;
    struct StreamDiagnosticConsumer;

    namespace builder {
        struct DiagnosticTokenBuilder;

        template <typename LocT>
        struct DiagnosticAnnotationBuilder;

        template <typename LocT>
        struct DiagnosticBuilder;

        struct AnnotatedStringBuilder;
    } // namespace builder
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_FORWARD_HPP
