#ifndef DARK_DIAGNOSTICS_BUILDER_CONTEXT_HPP
#define DARK_DIAGNOSTICS_BUILDER_CONTEXT_HPP

#include "../basic.hpp"
#include "../core/cow_string.hpp"
#include "../span.hpp"

namespace dark {
    namespace detail {
        template <typename T>
        concept ContextSpan = is_basic_span<T>;
    } // namespace detail 

    class DiagnosticContext {
        struct InternalBasicDiagnosticMessage {
            core::CowString message{};
            DiagnosticLevel level{};
            core::SmallVec<std::variant<Span, MarkerRelSpan, LocRelSpan>, 1> spans{};
            core::CowString text_to_be_inserted{};
            DiagnosticOperationKind op{ DiagnosticOperationKind::None };
        };
    public:
        auto insert(
            core::CowString text,
            typename Span::base_type pos,
            core::CowString message = {}
        ) -> DiagnosticContext& {
            auto span = Span::from_usize(pos, text.size());
            auto temp = InternalBasicDiagnosticMessage {
                .message = std::move(message),
                .spans = { span },
                .text_to_be_inserted = std::move(text),
                .op = DiagnosticOperationKind::Insert
            };
            context.push_back(std::move(temp));
            return *this;
        }

        auto insert_marker_rel(
            core::CowString text,
            typename MarkerRelSpan::base_type pos,
            core::CowString message = {}
        ) -> DiagnosticContext& {
            auto span = MarkerRelSpan::from_usize(pos, text.size());
            auto temp = InternalBasicDiagnosticMessage {
                .message = std::move(message),
                .spans = { span },
                .text_to_be_inserted = std::move(text),
                .op = DiagnosticOperationKind::Insert
            };
            context.push_back(std::move(temp));
            return *this;
        }

        auto insert_loc_rel(
            core::CowString text,
            typename LocRelSpan::base_type pos,
            core::CowString message = {}
        ) -> DiagnosticContext& {
            auto span = LocRelSpan::from_usize(pos, text.size());
            auto temp = InternalBasicDiagnosticMessage {
                .message = std::move(message),
                .spans = { span },
                .text_to_be_inserted = std::move(text),
                .op = DiagnosticOperationKind::Insert
            };
            context.push_back(std::move(temp));
            return *this;
        }
        auto del(detail::ContextSpan auto span, core::CowString message = {}) -> DiagnosticContext& {
            context.emplace_back(InternalBasicDiagnosticMessage{
                .message = std::move(message),
                .spans = { span },
                .op = DiagnosticOperationKind::Remove
            });
            return *this;
        }

        template<detail::ContextSpan... Ss>
            requires (sizeof...(Ss) > 0)
        auto error(Ss... spans) -> DiagnosticContext& {
            context.emplace_back(InternalBasicDiagnosticMessage{
                .level = DiagnosticLevel::Error,
                .spans = { std::forward<Ss>(spans)... },
            });
            return *this;
        }

        template<detail::ContextSpan... Ss>
        auto error(core::CowString message, Ss... spans) -> DiagnosticContext& {
            context.emplace_back(InternalBasicDiagnosticMessage{
                .message = std::move(message),
                .level = DiagnosticLevel::Error,
                .spans = { std::forward<Ss>(spans)... },
            });
            return *this;
        }

        template<detail::ContextSpan... Ss>
            requires (sizeof...(Ss) > 0)
        auto warn(Ss... spans) -> DiagnosticContext& {
            context.emplace_back(InternalBasicDiagnosticMessage{
                .level = DiagnosticLevel::Warning,
                .spans = { std::forward<Ss>(spans)... },
            });
            return *this;
        }

        template<detail::ContextSpan... Ss>
        auto warn(core::CowString message, Ss... spans) -> DiagnosticContext& {
            context.emplace_back(InternalBasicDiagnosticMessage{
                .message = std::move(message),
                .level = DiagnosticLevel::Warning,
                .spans = { std::forward<Ss>(spans)... },
            });
            return *this;
        }

        template<detail::ContextSpan... Ss>
            requires (sizeof...(Ss) > 0)
        auto remark(Ss... spans) -> DiagnosticContext& {
            context.emplace_back(InternalBasicDiagnosticMessage{
                .level = DiagnosticLevel::Remark,
                .spans = { std::forward<Ss>(spans)... },
            });
            return *this;
        }

        template<detail::ContextSpan... Ss>
        auto remark(core::CowString message, Ss... spans) -> DiagnosticContext& {
            context.emplace_back(InternalBasicDiagnosticMessage{
                .message = std::move(message),
                .level = DiagnosticLevel::Remark,
                .spans = { std::forward<Ss>(spans)... },
            });
            return *this;
        }

        template<detail::ContextSpan... Ss>
            requires (sizeof...(Ss) > 0)
        auto note(Ss... spans) -> DiagnosticContext& {
            context.emplace_back(InternalBasicDiagnosticMessage{
                .level = DiagnosticLevel::Note,
                .spans = { std::forward<Ss>(spans)... },
            });
            return *this;
        }

        template<detail::ContextSpan... Ss>
        auto note(core::CowString message, Ss... spans) -> DiagnosticContext& {
            context.emplace_back(InternalBasicDiagnosticMessage{
                .message = std::move(message),
                .level = DiagnosticLevel::Note,
                .spans = { std::forward<Ss>(spans)... },
            });
            return *this;
        }

        template <typename DiagnosticKindType>
        void assign_to_message(BasicDiagnostic<DiagnosticKindType>& message) {
            unsigned abs_position = message.location.get_absolute_position();
            auto marker_span = message.location.get_marker_span();
            for (auto& c: context) {
                core::SmallVec<Span, 1> spans;
                spans.reserve(c.spans.size());
                for (auto& span: c.spans) {
                    std::visit([abs_position, &spans, marker_span]<typename T>(T s) {
                        if constexpr (s.is_marker_relative()) {
                            auto t = s.resolve(marker_span.start());
                            if (s.empty()) {
                                t = marker_span;
                            }
                            if (t.empty()) return;
                            spans.push_back(t);
                        } else if constexpr (s.is_loc_relative()) {
                            auto t = s.resolve(abs_position); 
                            if (t.empty()) return;
                            spans.push_back(t);
                        } else {
                            if (s.empty()) return;
                            spans.push_back(s);
                        }
                    }, span);
                }

                auto temp = BasicDiagnosticMessage {
                    .message = std::move(c.message),
                    .level = c.level,
                    .spans = std::move(spans),
                    .text_to_be_inserted = std::move(c.text_to_be_inserted),
                    .op = c.op
                };
                message.context.push_back(std::move(temp));
            }
        }

        core::SmallVec<InternalBasicDiagnosticMessage, 2> context{};
    };
} // namespace dark

#endif // DARK_DIAGNOSTICS_BUILDER_CONTEXT_HPP
