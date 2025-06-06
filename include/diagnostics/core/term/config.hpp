#ifndef AMT_DARK_DIAGNOSTICS_CORE_TERM_CONFIG_HPP
#define AMT_DARK_DIAGNOSTICS_CORE_TERM_CONFIG_HPP

#include "basic.hpp"
#include <cstdlib>
#include <cstdio>

namespace dark::core::term {

    namespace detail {
        static inline auto check_terminal_environment_for_colors() noexcept -> bool {
            std::string_view info(std::getenv("TERM"));
            if (!info.empty()) {
                return (info == "ansi")
                || info == "cygwin"
                || info == "linux"
                || info.starts_with("tmux")
                || info.starts_with("screen")
                || info.starts_with("xterm")
                || info.starts_with("vt100")
                || info.starts_with("rxvt")
                || info.starts_with("color");
            }

            return false;
        }

        static inline auto terminal_has_color([[maybe_unused]] int fd) noexcept -> bool {
            #if defined(DARK_OS_UNIX) && defined(DARK_USE_TERMINFO)
                static std::mutex term_color_mutex;
                std::lock_guard lock(term_color_mutex);
                auto* previous_term = set_curterm(nullptr);
                int errret = 0;

                if (setupterm(nullptr, fd, &errret) != 0) return false;

                int colors_ti = tigetnum(const_cast<char *>("colors"));
                bool has_colors = colors_ti >= 0 ? static_cast<bool>(colors_ti) : check_terminal_environment_for_colors();
                auto *termp = set_curterm(previous_term);
                (void)del_curterm(termp);

                return has_colors;
            #else
                return check_terminal_environment_for_colors();
            #endif
        }

        static inline auto get_fd_from_handle(FILE* handle) noexcept -> int {
            #ifdef DARK_OS_UNIX
                return fileno(handle);
            #else
                return _fileno(handle);
            #endif
        }

        #ifdef DARK_OS_UNIX
            using native_handle_t = int;
        #else
            using native_handle_t = HANDLE;
        #endif

        static inline auto get_native_handle(FILE* handle) noexcept -> native_handle_t {
            auto fd = get_fd_from_handle(handle);
            #ifdef DARK_OS_UNIX
                return fd;
            #else
                HANDLE handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
                return handle;
            #endif
        }

        static inline auto get_columns_impl([[maybe_unused]] int fd) noexcept -> std::size_t {
            #ifdef DARK_OS_UNIX
                const char* cols_str = std::getenv("COLUMNS");
                if (cols_str) {
                    int cols = std::atoi(cols_str);
                    if (cols > 0) return static_cast<std::size_t>(cols);
                } else {
                    winsize win;
                    if (ioctl(fd, TIOCGWINSZ, &win) >= 0) {
                        return win.ws_col;
                    }
                }
                return 0zu;
            #else
                HANDLE handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
                unsigned cols = 0;
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                if (GetConsoleScreenBufferInfo(handle, &csbi)) cols = csbi.dwSize.X;
                return static_cast<std::size_t>(cols);
            #endif
        }
    } // detail

    static inline auto is_displayed(int fd) noexcept -> bool {
        #ifdef DARK_OS_UNIX
            return isatty(fd);
        #else
            DWORD Mode;
            return (GetConsoleMode(reinterpret_cast<HANDLE>(_get_osfhandle(fd)), &Mode) != 0);
        #endif
    }

    static inline auto supports_color(int fd) noexcept -> bool {
        #ifdef DARK_OS_UNIX
            return is_displayed(fd) && detail::terminal_has_color(fd);
        #else
            DWORD Mode;
            return (GetConsoleMode(reinterpret_cast<HANDLE>(_get_osfhandle(fd)), &Mode) != 0);
        #endif
    }

    static inline auto is_displayed(FILE* handle) noexcept -> bool {
        auto fd = detail::get_fd_from_handle(handle);
        return is_displayed(fd);
    }

    static inline auto supports_color(FILE* handle) noexcept -> bool {
        auto fd = detail::get_fd_from_handle(handle);
        return supports_color(fd);
    }


    static inline constexpr auto colors_need_flush() noexcept -> bool {
        #ifdef DARK_OS_UNIX
            return false;
        #else
            return !use_ansi;
        #endif
    }

    static inline auto use_ansi_escape_codes([[maybe_unused]] bool enable) noexcept {
        #ifdef DARK_WINDOWS
            #ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
                if (enable) {
                    HANDLE Console = GetStdHandle(fd_stdout);
                    DWORD Mode;
                    GetConsoleMode(Console, &Mode);
                    Mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    SetConsoleMode(Console, Mode);
                }
            #else
                use_ansi = enable;
            #endif
        #endif
    }

    static inline auto output_bold(bool bg) noexcept -> std::string_view {
        auto const code = (bg ? "\0x1b[1m" : "\x1b[0m");
        #ifdef DARK_OS_UNIX
            return code;
        #else
            if (use_ansi) return code;
            WORD colors = WindowsDefaultColors::get_current_color();
            if (bg) colors |= BACKGROUND_INTENSITY;
            else colors |= FOREGROUND_INTENSITY;

            SetConsoleTextAttribute(GetStdHandle(fd_stdout), colors);
        #endif
    }


    static inline auto output_color(TerminalStyle style) noexcept -> std::string_view {
        auto const code = style.to_ansi_code();
        #ifdef DARK_OS_UNIX
            return code;
        #else
            if (use_ansi) return code;
            WORD current = WindowsDefaultColors::get_current_color();
            WORD colors;
            if (style.bg) {
                colors = ((style.code & 1) ? BACKGROUND_RED : 0) |
                         ((style.code & 2) ? BACKGROUND_GREEN : 0) |
                         ((style.code & 4) ? BACKGROUND_BLUE : 0);

                if (style.bold) colors |= BACKGROUND_INTENSITY;
                colors |= fg_color(current);
            } else {
                colors = ((style.code & 1) ? FOREGROUND_RED : 0) |
                         ((style.code & 2) ? FOREGROUND_GREEN : 0) |
                         ((style.code & 4) ? FOREGROUND_BLUE : 0);
                if (style.bold) colors |= FOREGROUND_INTENSITY;
                colors |= bg_color(current);
            }
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colors);
            return {};
        #endif
    }

    static inline auto output_reverse() noexcept -> std::string_view {
        #ifdef DARK_OS_UNIX
            return "\033[7m";
        #else
            if (use_ansi) return "\033[7m";
            auto handle = GetStdHandle(fd_stdout);
            WORD const attributes = GetConsoleTextAttribute(handle);

            WORD const foreground_mask = FOREGROUND_BLUE | FOREGROUND_GREEN |
                                 FOREGROUND_RED | FOREGROUND_INTENSITY;
            WORD const background_mask = BACKGROUND_BLUE | BACKGROUND_GREEN |
                                 BACKGROUND_RED | BACKGROUND_INTENSITY;
            WORD const color_mask = foreground_mask | background_mask;

            WORD new_attributes =
                ((attributes & FOREGROUND_BLUE) ? BACKGROUND_BLUE : 0) |
                ((attributes & FOREGROUND_GREEN) ? BACKGROUND_GREEN : 0) |
                ((attributes & FOREGROUND_RED) ? BACKGROUND_RED : 0) |
                ((attributes & FOREGROUND_INTENSITY) ? BACKGROUND_INTENSITY : 0) |
                ((attributes & BACKGROUND_BLUE) ? FOREGROUND_BLUE : 0) |
                ((attributes & BACKGROUND_GREEN) ? FOREGROUND_GREEN : 0) |
                ((attributes & BACKGROUND_RED) ? FOREGROUND_RED : 0) |
                ((attributes & BACKGROUND_INTENSITY) ? FOREGROUND_INTENSITY : 0) | 0;
            new_attributes = (attributes & ~color_mask) | (new_attributes & color_mask);

            SetConsoleTextAttribute(handle, new_attributes);
            return {};
        #endif
    }

    static inline auto reset_color() noexcept -> std::string_view {
        #ifdef DARK_OS_UNIX
            return "\033[0m";
        #else
            if (use_ansi) return "\033[0m";
            SetConsoleTextAttribute(GetStdHandle(fd_stdout), windows_default_colors);
        #endif
    }

    static inline auto get_columns(int fd) noexcept -> std::size_t {
        if (!is_displayed(fd)) return 0ul;
        return detail::get_columns_impl(fd);
    }

    static inline auto get_columns(FILE* handle) noexcept -> std::size_t {
        auto fd = detail::get_fd_from_handle(handle);
        return get_columns(fd);
    }

    static inline auto supports_utf8() noexcept -> bool {
        auto tmp = std::string_view(std::getenv("LC_ALL"));
        if (tmp.empty()) {
            tmp = std::string_view(std::getenv("LC_CTYPE"));
            if (tmp.empty()) {
                tmp = std::string_view(std::getenv("LANG"));
            }
        }

        if (tmp.empty()) return false;
        return tmp.contains("UTF-8") || tmp.contains("utf-8");
    }
} // dark::core::term

#ifdef DARK_USE_TERMINFO
    #undef DARK_USE_TERMINFO
#endif

#endif // AMT_DARK_DIAGNOSTICS_CORE_TERM_CONFIG_HPP
