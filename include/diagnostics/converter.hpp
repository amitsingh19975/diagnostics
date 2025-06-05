#ifndef AMT_DARK_DIAGNOSTICS_CONVERTER_HPP
#define AMT_DARK_DIAGNOSTICS_CONVERTER_HPP

#include "basic.hpp"
#include "builders/diagnostic.hpp"
#include "forward.hpp"

namespace dark {
    template <typename LocT>
    struct DiagnosticConverter {
        using loc_t = LocT;
        using builder_t = builder::DiagnosticBuilder<LocT>;
        virtual ~DiagnosticConverter() = default;
        virtual auto convert_loc(LocT loc, builder_t& builder) const -> DiagnosticLocation = 0;
    };
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_CONVERTER_HPP
