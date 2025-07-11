#ifndef AMT_DARK_DIAGNOSTICS_CONSUMERS_STREAM_HPP
#define AMT_DARK_DIAGNOSTICS_CONSUMERS_STREAM_HPP

#include "base.hpp"
#include "../core/term/terminal.hpp"
#include "../core/term/config.hpp"
#include "../renderer.hpp"
#include <cassert>
#include <cstdio>

#ifdef DARK_OS_UNIX
    #include <fcntl.h>
#endif

namespace dark {
    struct StreamDiagnosticConsumer: DiagnosticConsumer {
    private:
        struct FileLock {
            FileLock(Terminal<FILE*>& out)
                : m_handle(out.get_native_handle())
            {
                #ifdef DARK_OS_UNIX
                    struct flock fl{};
                    fl.l_type = F_WRLCK;
                    fl.l_whence = SEEK_SET;
                    fl.l_start = 0;
                    fl.l_len = 0;
                    if (fcntl(m_handle, F_SETLKW, &fl) == -1) {
                        throw std::runtime_error("Failed to lock file");
                    }
                #else
                    if (!LockFileEx(m_handle, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ol)) {
                        throw std::runtime_error("Failed to lock file");
                    }
                #endif
            }

            ~FileLock() {
                #ifdef DARK_OS_UNIX
                    struct flock fl{};
                    fl.l_type = F_UNLCK;
                    fl.l_whence = SEEK_SET;
                    fl.l_start = 0;
                    fl.l_len = 0;
                    fcntl(m_handle, F_SETLK, &fl);
                #else
                    OVERLAPPED ol = {0};
                    UnlockFileEx(m_handle, 0, MAXDWORD, MAXDWORD, &ol);
                #endif
            }
        private:
            core::term::detail::native_handle_t m_handle;
        };
    public:
        explicit constexpr StreamDiagnosticConsumer(
            FILE* file,
            DiagnosticRenderConfig config = {}
        ) noexcept
            : m_out(file)
            , m_config(config)
        {}
        constexpr StreamDiagnosticConsumer(StreamDiagnosticConsumer const&) noexcept = default;
        constexpr StreamDiagnosticConsumer(StreamDiagnosticConsumer &&) noexcept = default;
        constexpr StreamDiagnosticConsumer& operator=(StreamDiagnosticConsumer const&) noexcept = default;
        constexpr StreamDiagnosticConsumer& operator=(StreamDiagnosticConsumer &&) noexcept = default;
        ~StreamDiagnosticConsumer() noexcept override = default;

        auto consume(Diagnostic&& d) -> void override {
            FileLock lock(m_out);
            render_diagnostic(m_out, d, m_config);
            m_out.write("\n");
        }

        auto flush() -> void override { m_out.flush(); }

        constexpr auto reset() noexcept -> void { m_has_printed = false; }

    private:
        Terminal<FILE*> m_out;
        DiagnosticRenderConfig m_config{};
        bool m_has_printed{false};
    };

    static inline auto ConsoleDiagnosticConsumer(
        DiagnosticRenderConfig config = {}
    ) -> DiagnosticConsumer* {
        static auto* consumer = new StreamDiagnosticConsumer(stderr, config);
        return consumer;
    }
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_CONSUMERS_STREAM_HPP
