#ifndef DARK_DIAGNOSTICS_CORE_TERMINAL_CONFIG_HPP
#define DARK_DIAGNOSTICS_CORE_TERMINAL_CONFIG_HPP

#include "config.hpp"
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <sys/ttycom.h>

#ifdef DARK_UNIX
    #include <unistd.h>
    #include <sys/ioctl.h>
#elif defined(DARK_WINDOWS)
    #include <Windows.h>
    #define STDIN_FILENO 0
    #define STDOUT_FILENO 1
    #define STDERR_FILENO 2
#else
    #error "Unknown platform"
#endif

#if defined(DARK_ENABLE_TERMINFO) && __has_include(<termios.h>) && __has_include(<term.h>)
    #include <termios.h>

    extern "C" int setupterm(char *term, int filedes, int *errret);
    extern "C" struct term *set_curterm(struct term *termp);
    extern "C" int del_curterm(struct term *termp);
    extern "C" int tigetnum(char *capname);
#else
    #undef DARK_ENABLE_TERMINFO
#endif

#define COLOR(FGBG, CODE, BOLD, DIM, STRIKE) "\033[0;" BOLD DIM STRIKE FGBG CODE "m"
#define COLOR3(BOLD, DIM, STRIKE) "\033[" BOLD DIM STRIKE "m"

#define ALLCOLORS(FGBG, BRIGHT, BOLD, DIM, STRIKE) \
  {                           \
    COLOR(FGBG, "0", BOLD, DIM, STRIKE),   \
    COLOR(FGBG, "1", BOLD, DIM, STRIKE),   \
    COLOR(FGBG, "2", BOLD, DIM, STRIKE),   \
    COLOR(FGBG, "3", BOLD, DIM, STRIKE),   \
    COLOR(FGBG, "4", BOLD, DIM, STRIKE),   \
    COLOR(FGBG, "5", BOLD, DIM, STRIKE),   \
    COLOR(FGBG, "6", BOLD, DIM, STRIKE),   \
    COLOR(FGBG, "7", BOLD, DIM, STRIKE),   \
    COLOR(BRIGHT, "0", BOLD, DIM, STRIKE), \
    COLOR(BRIGHT, "1", BOLD, DIM, STRIKE), \
    COLOR(BRIGHT, "2", BOLD, DIM, STRIKE), \
    COLOR(BRIGHT, "3", BOLD, DIM, STRIKE), \
    COLOR(BRIGHT, "4", BOLD, DIM, STRIKE), \
    COLOR(BRIGHT, "5", BOLD, DIM, STRIKE), \
    COLOR(BRIGHT, "6", BOLD, DIM, STRIKE), \
    COLOR(BRIGHT, "7", BOLD, DIM, STRIKE), \
    COLOR3(BOLD, DIM, STRIKE),             \
  }

namespace dark::core::internal {

    static constexpr std::string_view color_codes[2/*bg*/][2/*bold*/][2/*dim*/][2/*strike*/][17/*codes*/] = {
        // bg
        {
            // bold
            {
                // dim
                {
                    // bold 0, dim 0, strike 0
                    ALLCOLORS("3", "9", "", "", ""),

                    // bold 0, dim 0, strike 1
                    ALLCOLORS("3", "9", "", "", "9;"),
                },
                 
                {
                    // bold 0, dim 1, strike 0
                    ALLCOLORS("3", "9", "", "2;", ""),

                    // bold 0, dim 1, strike 1
                    ALLCOLORS("3", "9", "", "2;", "9;"),
                }

            },
            {
                // dim
                {
                    // bold 1, dim 0, strike 0
                    ALLCOLORS("3", "9", "1;", "", ""),

                    // bold 1, dim 0, strike 1
                    ALLCOLORS("3", "9", "1;", "", "9;"),
                },
                 
                {
                    // bold 1, dim 1, strike 0
                    ALLCOLORS("3", "9", "1;", "2;", ""),

                    // bold 1, dim 1, strike 1
                    ALLCOLORS("3", "9", "1;", "2;", "9;"),
                }

            }
        },
        {
            // bold
            {
                // dim
                {
                    // bold 0, dim 0, strike 0
                    ALLCOLORS("4", "9", "", "", ""),

                    // bold 0, dim 0, strike 1
                    ALLCOLORS("4", "9", "", "", "9;"),
                },
                 
                {
                    // bold 0, dim 1, strike 0
                    ALLCOLORS("4", "9", "", "2;", ""),

                    // bold 0, dim 1, strike 1
                    ALLCOLORS("4", "9", "", "2;", "9;"),
                }

            },
            {
                // dim
                {
                    // bold 1, dim 0, strike 0
                    ALLCOLORS("4", "9", "1;", "", ""),

                    // bold 1, dim 0, strike 1
                    ALLCOLORS("4", "9", "1;", "", "9;"),
                },
                 
                {
                    // bold 1, dim 1, strike 0
                    ALLCOLORS("4", "9", "1;", "2;", ""),

                    // bold 1, dim 1, strike 1
                    ALLCOLORS("4", "9", "1;", "2;", "9;"),
                }

            }
        }
    };

    struct TerminalStyle {
        bool bg{};
        bool bold{};
        bool dim{};
        bool strike{};
        char code{16};
    };

    constexpr std::string_view get_color_code(TerminalStyle style) noexcept {
        return color_codes[
            static_cast<std::size_t>(style.bg)
        ][
            static_cast<std::size_t>(style.bold)
        ][
            static_cast<std::size_t>(style.dim)
        ][
            static_cast<std::size_t>(style.strike)
        ][
            static_cast<std::size_t>(style.code % 17)
        ];
    }; 

    #ifdef DARK_WINDOWS
        static bool use_ansi = false;
        namespace {
            class WindowsDefaultColors {
            public:
                WindowsDefaultColors()
                    : m_defaultColor(get_current_color())
                {}

                static unsigned get_current_color() {
                    CONSOLE_SCREEN_BUFFER_INFO csbi;
                    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) return csbi.wAttributes;
                    return 0;
                }
                WORD operator()() const { return m_default_color; }
            private:
                WORD m_default_color;
            };

            WindowsDefaultColors windows_default_colors;

            WORD fg_color(WORD color) {
                return color & (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY |
                                FOREGROUND_RED);
            }

            WORD bg_color(WORD color) {
                return color & (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY |
                                BACKGROUND_RED);
            }
        } // namespace
    #endif

    struct TerminalConfig {
        auto is_displayed(int fd) const noexcept -> bool {
            #ifdef DARK_UNIX
                return isatty(fd);
            #else
                DWORD Mode;
                return (GetConsoleMode(reinterpret_cast<HANDLE>(_get_osfhandle(fd)), &Mode) != 0);
            #endif
        }

        auto supports_color(int fd) const noexcept -> bool {
            #ifdef DARK_UNIX
                return is_displayed(fd) && terminal_has_color(fd);
            #else
                return is_displayed(fd);
            #endif
        }

        auto get_fd_from_handle(FILE* handle) const noexcept -> int {
            #ifdef DARK_UNIX
                return fileno(handle);
            #else
                return _fileno(handle);
            #endif
        }

        constexpr auto colors_need_flush() const noexcept -> bool {
            #ifdef DARK_UNIX
                return false;
            #else
                return !use_ansi;
            #endif
        }

        auto use_ansi_escape_codes([[maybe_unused]] bool enable) const noexcept {
            #ifdef DARK_WINDOWS
                #ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
                    if (enable) {
                        HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);
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

        auto output_bold(bool bg) const noexcept -> std::string_view {
            auto const ascii = (bg ? "\033[1m" : "\033[0m");
            #ifdef DARK_UNIX
                return ascii;
            #else
                if (use_ansi) return ascii; 
                WORD colors = WindowsDefaultColors::get_current_color();
                if (bg)
                   colors |= BACKGROUND_INTENSITY;
                 else
                   colors |= FOREGROUND_INTENSITY;
                 SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colors);
                return {};
            #endif
        }

        auto output_color(TerminalStyle style) const noexcept -> std::string_view {
            #ifdef DARK_UNIX
                return get_color_code(style);
            #else
                if (use_ansi) return get_color_code(style);
                WORD current = WindowsDefaultColors::get_current_color();
                WORD colors;
                if (bg) {
                    colors = ((code & 1) ? BACKGROUND_RED : 0) |
                            ((code & 2) ? BACKGROUND_GREEN : 0) |
                            ((code & 4) ? BACKGROUND_BLUE : 0);

                    if (bold) colors |= BACKGROUND_INTENSITY;
                    colors |= fg_color(current);
                } else {
                    colors = ((code & 1) ? FOREGROUND_RED : 0) |
                            ((code & 2) ? FOREGROUND_GREEN : 0) |
                            ((code & 4) ? FOREGROUND_BLUE : 0);
                    if (bold) colors |= FOREGROUND_INTENSITY;
                    colors |= bg_color(current);
                }
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colors);
                return {};
            #endif
        }

        auto output_reverse() const noexcept -> std::string_view {
            #ifdef DARK_UNIX
                return "\033[7m";
            #else
                if (use_ansi) return "\033[7m";
                const WORD attributes =
                      GetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE));

                  const WORD foreground_mask = FOREGROUND_BLUE | FOREGROUND_GREEN |
                                               FOREGROUND_RED | FOREGROUND_INTENSITY;
                  const WORD background_mask = BACKGROUND_BLUE | BACKGROUND_GREEN |
                                               BACKGROUND_RED | BACKGROUND_INTENSITY;
                  const WORD color_mask = foreground_mask | background_mask;

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

                  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), new_attributes);
                  return {};
            #endif
        }

        auto reset_color() const noexcept -> std::string_view {
            #ifdef DARK_UNIX
                return "\033[0m";
            #else
                if (use_ansi) return "\033[0m";
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), windows_default_colors());
            #endif
        }

        auto get_columns(int fd) const noexcept -> std::size_t {
            if (!is_displayed(fd)) return 0u;
            return get_columns_helper(fd);
        }

    private:
        auto terminal_has_color([[maybe_unused]] int fd) const noexcept -> bool {
            #if defined(DARK_UNIX) && defined(DARK_ENABLE_TERMINFO)
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

        auto check_terminal_environment_for_colors() const noexcept -> bool {
            std::string_view info(std::getenv("TERM"));
            if (!info.empty()) {
                return (info == "ansi")
                || info == "cygwin"
                || info == "linux"
                || info.starts_with("screen")
                || info.starts_with("xterm")
                || info.starts_with("vt100")
                || info.starts_with("rxvt")
                || info.starts_with("color");
            }

            return false;
        }

        auto get_columns_helper([[maybe_unused]] int fd) const noexcept -> std::size_t {
            #ifdef DARK_UNIX
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
    };

    static constexpr auto terminal_config = TerminalConfig{};
} // namespace dark::core::internal

#undef COLOR
#undef ALLCOLORS

#endif // DARK_DIAGNOSTICS_CORE_TERMINAL_CONFIG_HPP
