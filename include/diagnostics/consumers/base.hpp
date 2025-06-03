#ifndef AMT_DARK_DIAGNOSTICS_CONSUMERS_BASE_HPP
#define AMT_DARK_DIAGNOSTICS_CONSUMERS_BASE_HPP

#include "../basic.hpp"
#include <cassert>

namespace dark {
    struct DiagnosticConsumer {
        virtual ~DiagnosticConsumer() = default;
        virtual auto consume(Diagnostic&& diagnostic) -> void = 0;
        virtual auto flush() -> void {}
    };
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_CONSUMERS_BASE_HPP
