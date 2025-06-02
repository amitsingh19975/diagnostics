#ifndef AMT_DARK_DIAGNOSTIC_CORE_TERM_BASIC_HPP
#define AMT_DARK_DIAGNOSTIC_CORE_TERM_BASIC_HPP

#include "../config.hpp"
#include <string_view>

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


#define COLOR(FGBG, CODE, BOLD, DIM, STRIKE) "\033[" BOLD DIM STRIKE FGBG CODE "m"
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
        using size_type = std::size_t;

        bool bg{}; // background
        bool bold{};
        bool dim{};
        bool strike{};
        char code{16};

        constexpr auto to_ansi_code() const noexcept -> std::string_view {
            auto bg_ = static_cast<size_type>(bg);
            auto bold_ = static_cast<size_type>(bold);
            auto dim_ = static_cast<size_type>(dim);
            auto strike_ = static_cast<size_type>(strike);
            auto code_ = static_cast<size_type>(code) % 17;
            return color_codes[bg_][bold_][dim_][strike_][code_];
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

#endif // AMT_DARK_DIAGNOSTIC_CORE_TERM_BASIC_HPP
