#ifndef AMT_DARK_DIAGNOSTIC_CORE_TERMINAL_HPP
#define AMT_DARK_DIAGNOSTIC_CORE_TERMINAL_HPP

#include "config.hpp"
#include <cstdint>
#include <cstdio>
#include <print>
#include <tuple>

namespace dark {

    struct Terminal {
        enum class ColorMode {
            Disable = 0,
            Enable,
            Auto
        };

        struct Style {
            bool bold{false};
            bool dim{false};
            bool strike{false};
            bool italic{false};

            constexpr auto operator==(Style const&) const noexcept -> bool = default;
        };

        static constexpr auto default_style() noexcept -> Style {
            return Style { .bold = false, .dim = false, .strike = false, .italic = false };
        };

        enum class Color: std::uint8_t {
            BLACK = 0,
            RED,
            GREEN,
            YELLOW,
            BLUE,
            MAGENTA,
            CYAN,
            WHITE,
            SAVEDCOLOR,
            RESET
        };

        static constexpr Color BLACK = Color::BLACK;
        static constexpr Color RED = Color::RED;
        static constexpr Color GREEN = Color::GREEN;
        static constexpr Color YELLOW = Color::YELLOW;
        static constexpr Color BLUE = Color::BLUE;
        static constexpr Color MAGENTA = Color::MAGENTA;
        static constexpr Color CYAN = Color::CYAN;
        static constexpr Color WHITE = Color::WHITE;
        static constexpr Color SAVEDCOLOR = Color::SAVEDCOLOR;
        static constexpr Color RESET = Color::RESET;

        Terminal(
            FILE* handle = stdin,
            ColorMode mode = ColorMode::Auto
        ) noexcept
            : m_handle(handle)
            , m_color_enabled(supports_color(handle, mode))
        {}

        constexpr Terminal(Terminal const&) noexcept = default;
        constexpr Terminal(Terminal &&) noexcept = default;
        constexpr Terminal& operator=(Terminal const&) noexcept = default;
        constexpr Terminal& operator=(Terminal &&) noexcept = default;

        constexpr auto get_handle() noexcept -> FILE* {
            return m_handle;
        }

        constexpr auto get_native_handle() noexcept -> core::term::detail::native_handle_t {
            return core::term::detail::get_native_handle(get_handle());
        }

        auto write(std::string_view str) -> Terminal& {
            std::print(m_handle, "{}", str);
            return *this;
        }

        template <typename... Args>
        auto write(std::format_string<Args...> fmt, Args&&... args) -> Terminal& {
            std::print(m_handle, fmt, std::forward<Args>(args)...);
            return *this;
        }

        auto write(
            std::string_view str,
            Color textColor,
            Color bgColor = SAVEDCOLOR,
            Style style = default_style()
        ) -> Terminal& {
            change_color(textColor, bgColor, style);
            write(str);
            reset_color();
            return *this;
        }

        auto flush() noexcept {
            fflush(m_handle);
        }

        auto is_displayed() const noexcept -> bool {
            return core::term::is_displayed(m_handle);
        }

        auto prepare_colors() noexcept -> bool {
            if (!m_color_enabled) return false;
            if (core::term::colors_need_flush() && !is_displayed()) return false;
            if (core::term::colors_need_flush()) flush();
            return true;
        }

        auto change_color(
            Color text_color,
            Color bg_color,
            Style style = default_style()
        ) -> Terminal& {
            if (!prepare_colors()) return *this;

            auto new_style = std::make_tuple(style, text_color, bg_color);

            if (m_current_style == new_style) return *this;
            m_current_style = new_style;

            if (bg_color != SAVEDCOLOR) {
                auto term_style = core::term::TerminalStyle {
                    .bg = true,
                };

                term_style.code = static_cast<char>(bg_color);
                auto code = core::term::output_color(term_style);
                write(code);
            }

            {
                auto term_style = core::term::TerminalStyle {
                    .bg = false,
                    .bold = style.bold,
                    .dim = style.dim,
                    .strike = style.strike,
                    .italic = style.italic
                };

                if (text_color != SAVEDCOLOR) {
                    term_style.code = static_cast<char>(text_color);
                }
                auto code = core::term::output_color(term_style);
                write(code);
            }
            return *this;
        }

        auto change_color(
            Color text_color,
            Style style = default_style()
        ) -> Terminal& {
            return change_color(text_color, SAVEDCOLOR, style);
        }

        auto reset_color() -> Terminal& {
            if (!prepare_colors()) return *this;

            auto new_style = std::make_tuple(default_style(), SAVEDCOLOR, SAVEDCOLOR);
            if (m_current_style == new_style) {
                return *this;
            }
            write(core::term::reset_color());
            m_current_style = new_style;
            return *this;
        }

        auto reverse_color() -> Terminal& {
            if (!prepare_colors()) return *this;
            write(core::term::output_reverse());
            auto [style, text_color, bg_color] = m_current_style;
            auto new_style = std::make_tuple(style, bg_color, text_color);
            m_current_style = new_style;
            return *this;
        }

        constexpr auto enable_colors(bool enable) noexcept {
            m_color_enabled = enable;
        }

        auto columns() noexcept -> std::size_t {
            return core::term::get_columns(m_handle);
        }

        ~Terminal() noexcept {
            flush();
        }

    private:
        static auto supports_color(FILE* handle, ColorMode mode) noexcept -> bool {
            if (mode != ColorMode::Auto) return static_cast<bool>(mode);
            return core::term::supports_color(handle);
        }

    private:
        FILE* m_handle;
        bool m_color_enabled{false};
        std::tuple<Style, Color, Color> m_current_style{
            default_style(), // style
            SAVEDCOLOR, // text color
            SAVEDCOLOR // bg color
        };
    };

} // namespace dark

#endif // AMT_DARK_DIAGNOSTIC_CORE_TERMINAL_HPP
