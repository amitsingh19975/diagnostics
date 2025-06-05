#ifndef AMT_DARK_DIAGNOSTICS_EMITTER_HPP
#define AMT_DARK_DIAGNOSTICS_EMITTER_HPP

#include "converter.hpp"
#include "consumers/base.hpp"
#include "core/small_vec.hpp"
#include "core/function_ref.hpp"
#include "builders/diagnostic.hpp"
#include "diagnostics/basic.hpp"
#include "diagnostics/core/format_any.hpp"
#include "forward.hpp"

namespace dark {
    template <typename LocT>
    struct DiagnosticEmitter {
        using builder_t = builder::DiagnosticBuilder<LocT>;

        constexpr DiagnosticEmitter(
            DiagnosticConverter<LocT>* converter,
            DiagnosticConsumer* consumer
        )
            : m_converter(converter)
            , m_consumer(consumer)
        {
            assert(converter != nullptr);
            assert(consumer != nullptr);
        }

        template <core::IsFormattable... Args>
        [[nodiscard("Missing `emit()` call")]] auto error(LocT loc, internal::DiagnosticBase<Args...> const& base, Args... args) -> builder_t {
            return builder_t(
                this,
                loc,
                DiagnosticLevel::Error,
                base,
                base.apply(std::forward<Args>(args)...)
            );
        }

        template <core::IsFormattable... Args>
        [[nodiscard("Missing `emit()` call")]] auto warn(LocT loc, internal::DiagnosticBase<Args...> const& base, Args... args) -> builder_t {
            return builder_t(
                this,
                loc,
                DiagnosticLevel::Warning,
                base,
                base.apply(std::forward<Args>(args)...)
            );
        }

        ~DiagnosticEmitter() { m_consumer->flush(); }
    private:
        template <typename L, typename A>
        friend struct DiagnosticAnnotationScope;

        friend struct DiagnosticEmitter<LocT>;
        friend struct builder::DiagnosticBuilder<LocT>;
    private:
        DiagnosticConverter<LocT>* m_converter;
        DiagnosticConsumer* m_consumer;
        core::SmallVec<core::FunctionRef<void(builder_t&)>> m_annotations;
    };

    template <typename LocT>
    DiagnosticEmitter(DiagnosticConverter<LocT>& converter, DiagnosticConsumer* consumer) noexcept -> DiagnosticEmitter<LocT>;

    template <typename LocT, typename AnnotateFn>
    struct DiagnosticAnnotationScope {
        constexpr DiagnosticAnnotationScope(
            DiagnosticEmitter<LocT>* emitter,
            AnnotateFn annotate
        ) noexcept
            : m_emitter(&emitter)
            , m_annotate_fn(std::move(annotate))
        {
            m_emitter->m_annotations.push_back(m_annotate_fn);
        }

        ~DiagnosticAnnotationScope() {
            m_emitter->m_annotations.pop_back();
        }
    private:
        DiagnosticEmitter<LocT>* m_emitter;
        AnnotateFn m_annotate_fn;
    };
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_EMITTER_HPP
