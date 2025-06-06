#ifndef AMT_DARK_DIAGNOSTICS_CORE_TERM_BASIC_HPP
#define AMT_DARK_DIAGNOSTICS_CORE_TERM_BASIC_HPP

#include "../config.hpp"
#include <algorithm>
#include <format>
#include <functional>
#include <numeric>
#include <string_view>
#include <array>
#include <utility>

#ifdef DARK_OS_UNIX
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <sys/ttycom.h>
#elifdef DARK_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#    define WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#    define NOMINMAX_WAS_LOCALLY_DEFINED
#  endif
#
#  include <windows.h>
#
#  ifdef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#    undef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#    undef WIN32_LEAN_AND_MEAN
#  endif
#  ifdef NOMINMAX_WAS_LOCALLY_DEFINED
#    undef NOMINMAX_WAS_LOCALLY_DEFINED
#    undef NOMINMAX
#  endif
#else
    #error "Unsupported platform"
#endif

#if defined(DARK_ENABLE_TERMINFO) && __has_include(<termios.h>) && __has_include(<term.h>)
    #define DARK_USE_TERMINFO

    #include <termios.h>
    #ifdef DARK_OS_UNIX
        #include <thread>
        #include <mutex>
    #endif

    extern "C" int setupterm(char *term, int filedes, int *errret);
    extern "C" struct term *set_curterm(struct term *termp);
    extern "C" int del_curterm(struct term *termp);
    extern "C" int tigetnum(char *capname);
#endif

namespace dark::core::term {

#ifdef DARK_OS_UNIX
    static constexpr int fd_stdin = STDIN_FILENO;
    static constexpr int fd_stdout = STDOUT_FILENO;
    static constexpr int fd_stderr = STDERR_FILENO;
#elifdef DARK_OS_WIN
    static constexpr DWORD fd_stdin = STD_INPUT_HANDLE;
    static constexpr DWORD fd_stdout = STD_OUTPUT_HANDLE;
    static constexpr DWORD fd_stderr = STD_ERROR_HANDLE;
#endif

    struct TerminalStyle {
        using size_type = std::size_t;

        bool bg{}; // background
        bool bold{};
        bool dim{};
        bool strike{};
        bool italic{};
        bool reset{};
        char code{16};

        constexpr auto to_ansi_code() const noexcept -> std::string_view {
            thread_local static char buffer[32] = {0};
            auto it = std::format_to(buffer, "\x1b[");
            bool first = true;
            auto append = [&](std::string_view s) {
                if (!first) {
                    it = std::format_to(it, "{};", s);
                } else {
                    it = std::format_to(it, "{}", s);
                }
                first = false;
            };

            if (bold)   append("1");
            if (dim)    append("2");
            if (italic) append("3");
            if (strike) append("9");

            auto code_ = static_cast<size_type>(code) % 17;
            if (bg) {
                if (reset) code_ = 49;
                else if (code_ < 8) code_ += 40;
                else code_ = 90 + code_ % 8;
            } else {
                if (reset) code_ = 39;
                else if (code_ < 8) code_ += 30;
                else code_ = 100 + code_ % 8;
            }

            if (code < 16) {
                if (!first) {
                    it = std::format_to(it, ";");
                }
                it = std::format_to(it, "{}", code_);
                first = false;
            }

            if (first) return {};
            it = std::format_to(it, "m");
            auto diff = static_cast<size_type>(it - buffer);
            return { buffer, diff };
        }
    };

    #ifdef DARK_OS_WIN
        static bool use_ansi = false;
        namespace {
            class WindowsDefaultColors {
            public:
                WindowsDefaultColors()
                    : m_defaultColor(get_current_color())
                {}

                static unsigned get_current_color() {
                    CONSOLE_SCREEN_BUFFER_INFO csbi;
                    if (GetConsoleScreenBufferInfo(GetStdHandle(fd_stdout), &csbi)) return csbi.wAttributes;
                    return 0;
                }
                WORD operator()() const { return m_default_color; }
            private:
                WORD m_default_color;
            };

            static WindowsDefaultColors windows_default_colors{};

            static inline auto fg_color(WORD color) noexcept -> WORD {
                return color & (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY |
                                FOREGROUND_RED);
            }

            static inline auto bg_color(WORD color) noexcept -> WORD {
                return color & (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY |
                                BACKGROUND_RED);
            }
        } // namespace
    #endif

} // namespace dark::core::term

#undef COLOR
#undef COLOR3
#undef ALLCOLORS

#endif // AMT_DARK_DIAGNOSTICS_CORE_TERM_BASIC_HPP
