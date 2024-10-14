#ifndef DARK_DIAGNOSTICS_EMITTER_HPP
#define DARK_DIAGNOSTICS_EMITTER_HPP

#include "consumer.hpp"
#include "core/function_ref.hpp"
#include "core/small_vector.hpp"
#include "diagnostics/basic.hpp"
#include "line_converter.hpp"
#include <cassert>
#include <utility>
#include "builder/sub_diagnostic.hpp"
#include "builder/diagnostic.hpp"

namespace dark {

    template <typename LocT, typename DiagnosticKindType, typename AnnotateFn>
    class DiagnosticAnnotationScope;

    namespace internal {
        template <typename LocT, typename DiagnosticKindType>
        class SubDiagnosticBuilder;

        template <typename LocT, typename DiagnosticKindType>
        class DiagnosticBuilder;
    } // namespace internal

    template <typename LocT, typename DiagnosticKindType>
    class BasicDiagnosticEmitter {
        using builder_t = internal::DiagnosticBuilder<LocT, DiagnosticKindType>;
    public:

        BasicDiagnosticEmitter(
            BasicDiagnosticConverter<LocT, DiagnosticKindType>& converter,
            DiagnosticConsumer<DiagnosticKindType>& consumer
        ) noexcept
            : m_converter(&converter)
            , m_consumer(&consumer)
        {}

        template<typename... Args, core::detail::is_valid_formatter_arg... Ts>
            requires ((sizeof...(Args) == sizeof...(Ts)) && (internal::is_format_arg_compatible<Args, Ts>::value && ...))
        auto error(LocT loc, internal::DiagnosticBase<DiagnosticKindType, Args...> const& base, Ts&&... args) -> builder_t {
            return builder_t(this, loc, DiagnosticLevel::Error, base, base.apply(std::forward<Ts>(args)...));
        }

        template<typename... Args, core::detail::is_valid_formatter_arg... Ts>
            requires ((sizeof...(Args) == sizeof...(Ts)) && (internal::is_format_arg_compatible<Args, Ts>::value && ...))
        auto warn(LocT loc, internal::DiagnosticBase<DiagnosticKindType, Args...> const& base, Ts&&... args) -> builder_t {
            return builder_t(this, loc, DiagnosticLevel::Warning, base, base.apply(std::forward<Ts>(args)...));
        }

        template<typename... Args, core::detail::is_valid_formatter_arg... Ts>
            requires ((sizeof...(Args) == sizeof...(Ts)) && (internal::is_format_arg_compatible<Args, Ts>::value && ...))
        auto remark(LocT loc, internal::DiagnosticBase<DiagnosticKindType, Args...> const& base, Ts&&... args) -> builder_t {
            return builder_t(this, loc, DiagnosticLevel::Remark, base, base.apply(std::forward<Ts>(args)...));
        }

    private:
        friend class BasicDiagnosticEmitter<LocT, DiagnosticKindType>;
        template <typename, typename, typename> friend class DiagnosticAnnotationScope;

        friend class internal::DiagnosticBuilder<LocT, DiagnosticKindType>;
        friend class internal::SubDiagnosticBuilder<LocT, DiagnosticKindType>;
    private:
        BasicDiagnosticConverter<LocT, DiagnosticKindType>* m_converter;
        DiagnosticConsumer<DiagnosticKindType>* m_consumer;
        core::SmallVec<core::FunctionRef<void(builder_t&)>> m_annotations;
    };

    template <typename LocT, typename DiagnosticKindType>
    BasicDiagnosticEmitter(BasicDiagnosticConverter<LocT>& converter, DiagnosticConsumer<DiagnosticKindType>& consumer) noexcept -> BasicDiagnosticEmitter<LocT, DiagnosticKindType>;

    template <typename LocT, typename DiagnosticKindType, typename AnnotateFn>
    class DiagnosticAnnotationScope {
        constexpr DiagnosticAnnotationScope(
            BasicDiagnosticEmitter<LocT, DiagnosticKindType>& emitter,
            AnnotateFn annotate
        ) noexcept
            : m_emitter(&emitter)
            , m_annotate(std::move(annotate))
        {
            m_emitter->m_annotations.push_back(m_annotate);
        }

        ~DiagnosticAnnotationScope() {
            m_emitter->m_annotations->pop_back();
        }
    public:
    private:
        BasicDiagnosticEmitter<LocT, DiagnosticKindType>* m_emitter;
        AnnotateFn m_annotate;
    };

} // namespace dark

#endif // DARK_DIAGNOSTICS_LINE_CONVERTER_HPP
