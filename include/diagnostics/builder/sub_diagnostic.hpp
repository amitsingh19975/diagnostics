#ifndef DARK_DIAGNOSTICS_BUILDER_SUB_DIAGNOSTIC_HPP
#define DARK_DIAGNOSTICS_BUILDER_SUB_DIAGNOSTIC_HPP

#include "../builder/context.hpp"

namespace dark {
    template <typename LocT, typename DiagnosticKindType>
    class DiagnosticBuilder;
} // namespace dark

namespace dark::internal {

    template <typename LocT, typename DiagnosticKindType>
    class SubDiagnosticBuilder {
    public:
        constexpr SubDiagnosticBuilder(SubDiagnosticBuilder const&) noexcept = delete;
        constexpr SubDiagnosticBuilder& operator=(SubDiagnosticBuilder const&) noexcept = delete;
        constexpr SubDiagnosticBuilder(SubDiagnosticBuilder&&) noexcept = default;
        SubDiagnosticBuilder& operator=(SubDiagnosticBuilder&&) noexcept = default;
        constexpr ~SubDiagnosticBuilder() = default;

        auto build() -> DiagnosticBuilder<LocT, DiagnosticKindType>& {
            if (!m_message.formatter.empty()) {
                get_sub_diagnostic().push_back(std::move(m_message));
            }
            return m_builder;
        }

        auto context(DiagnosticContext context) -> SubDiagnosticBuilder& {
            context.assign_to_message(get_sub_diagnostic().back());
            return *this;
        }

        template<typename... Args, core::detail::is_valid_formatter_arg... Ts>
            requires ((sizeof...(Args) == sizeof...(Ts)) && (internal::is_format_arg_compatible<Args, Ts>::value && ...))
        auto error(LocT loc, internal::DiagnosticBase<DiagnosticKindType, Args...> const& base, Ts&&... args) -> SubDiagnosticBuilder& {
            auto formatter = base.apply(std::forward<Ts>(args)...);
            add_message(std::move(loc), DiagnosticLevel::Error, base, std::move(formatter));
            return *this;
        }

        template<typename... Args, core::detail::is_valid_formatter_arg... Ts>
            requires ((sizeof...(Args) == sizeof...(Ts)) && (internal::is_format_arg_compatible<Args, Ts>::value && ...))
        auto warn(LocT loc, internal::DiagnosticBase<DiagnosticKindType, Args...> const& base, Ts&&... args) -> SubDiagnosticBuilder& {
            auto formatter = base.apply(std::forward<Ts>(args)...);
            add_message(std::move(loc), DiagnosticLevel::Warning, base, std::move(formatter));
            return *this;
        }

        template<typename... Args, core::detail::is_valid_formatter_arg... Ts>
            requires ((sizeof...(Args) == sizeof...(Ts)) && (internal::is_format_arg_compatible<Args, Ts>::value && ...))
        auto remark(LocT loc, internal::DiagnosticBase<DiagnosticKindType, Args...> const& base, Ts&&... args) -> SubDiagnosticBuilder& {
            auto formatter = base.apply(std::forward<Ts>(args)...);
            add_message(std::move(loc), DiagnosticLevel::Remark, base, std::move(formatter));
            return *this;
        }

    private:
        friend class DiagnosticBuilder<LocT, DiagnosticKindType>;

    private:
        template <typename... Args>
        SubDiagnosticBuilder(
            DiagnosticBuilder<LocT, DiagnosticKindType>& base_builder
        )
            : m_builder(base_builder)
        {}

        constexpr auto& get_sub_diagnostic() noexcept {
            return m_builder.m_diagnostic.sub_diagnostic;
        }

        template <typename... Args>
        auto add_message(
            LocT loc,
            DiagnosticLevel level,
            internal::DiagnosticBase<DiagnosticKindType, Args...> const& base,
            core::Formatter formatter
        ) -> void {
            add_message_with_loc(
                level,
                m_builder.m_emitter->m_converter->convert_loc(loc, *this),
                base,
                std::move(formatter)
            );
        }

        template <typename... Args>
        auto add_message_with_loc(
            DiagnosticLevel level,
            DiagnosticLocation location,
            internal::DiagnosticBase<DiagnosticKindType, Args...> const& base,
            core::Formatter formatter
        ) -> void {
            m_message.kind = std::move(base.kind);
            m_message.level = level;
            m_message.formatter = std::move(formatter);
            m_message.location = std::move(location);
        }
    private:
        DiagnosticBuilder<LocT, DiagnosticKindType>& m_builder;
        SubDiagnosticMessage<DiagnosticKindType> m_message{};
    };

} // namespace dark::internal

#endif // DARK_DIAGNOSTICS_BUILDER_SUB_DIAGNOSTIC_HPP
