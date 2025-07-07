#ifndef AMT_DARK_DIAGNOSTICS_CORE_TERMINAL_HPP
#define AMT_DARK_DIAGNOSTICS_CORE_TERMINAL_HPP

#include "config.hpp"
#include "color.hpp"
#include "writer.hpp"
#include <cstdio>
#include <print>
#include <tuple>


namespace dark {

    enum class TerminalColorMode {
        Disable = 0,
        Enable,
        Auto
    };

    template <typename T>
    struct Terminal {
        using writer_t = Writer<T>;

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

        Terminal(
            FILE* handle = stdin,
            TerminalColorMode mode = TerminalColorMode::Auto
        ) noexcept
            : m_writer(handle)
            , m_color_enabled(supports_color(handle, mode))
        {}

        Terminal(
            Writer<T> writer,
            TerminalColorMode mode = TerminalColorMode::Disable
        ) noexcept
            : m_writer(std::move(writer))
            , m_color_enabled(mode == TerminalColorMode::Enable)
        {}

        constexpr Terminal(Terminal const&) noexcept = default;
        constexpr Terminal(Terminal &&) noexcept = default;
        constexpr Terminal& operator=(Terminal const&) noexcept = default;
        constexpr Terminal& operator=(Terminal &&) noexcept = default;

        constexpr auto get_handle() noexcept -> FILE* requires (detail::WriterHasHandle<T>) {
            return m_writer.get_handle();
        }

        constexpr auto get_native_handle() noexcept -> core::term::detail::native_handle_t requires (detail::WriterHasHandle<T>) {
            return core::term::detail::get_native_handle(get_handle());
        }

        auto write(std::string_view str) -> Terminal& {
            m_writer.write(str);
            return *this;
        }

        template <typename... Args>
        auto write(std::format_string<Args...> fmt, Args&&... args) -> Terminal& {
            if constexpr (requires {
                { m_writer.write(fmt, std::forward<Args>(args)...) } -> std::same_as<void>;
            }) {
                m_writer.write(fmt, std::forward<Args>(args)...);
            } else {
                auto tmp = std::format(fmt, std::forward<Args>(args)...);
                write(tmp);
            }
            return *this;
        }

        auto write(
            std::string_view str,
            Color textColor,
            Color bgColor = Color::Default,
            Style style = default_style()
        ) -> Terminal& {
            change_color(textColor, bgColor, style);
            write(str);
            reset_color();
            return *this;
        }

        auto flush() noexcept {
            m_writer.flush();
        }

        auto is_displayed() const noexcept -> bool {
            return m_writer.is_displayed();
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
            auto old_style = std::get<0>(m_current_style);
            auto has_attribute_changed = (
                (old_style.bold != style.bold) ||
                (old_style.dim != style.dim) ||
                (old_style.italic != style.italic) ||
                (old_style.strike != style.strike)
            );
            if (has_attribute_changed) reset_color();
            auto should_render_bg_color = std::get<2>(m_current_style) != bg_color;
            auto should_render_text_color = std::get<1>(m_current_style) != text_color;
            m_current_style = new_style;

            if (should_render_bg_color) {
                auto term_style = core::term::TerminalStyle {
                    .bg = true,
                    .reset = bg_color == Color::Default,
                    .color = bg_color
                };

                auto code = core::term::output_color(term_style);
                write(code);
            }

            {
                auto term_style = core::term::TerminalStyle {
                    .bg = false,
                    .bold = style.bold,
                    .dim = style.dim,
                    .strike = style.strike,
                    .italic = style.italic,
                    .reset = should_render_text_color && text_color == Color::Default,
                };

                if (should_render_text_color) {
                    term_style.color = text_color;
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
            return change_color(text_color, Color::Default, style);
        }

        auto reset_color() -> Terminal& {
            if (!prepare_colors()) return *this;

            auto new_style = std::make_tuple(default_style(), Color::Default, Color::Default);
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
            return m_writer.columns();
        }

        ~Terminal() noexcept {
            flush();
        }

    private:
        static auto supports_color(FILE* handle, TerminalColorMode mode) noexcept -> bool {
            if (mode != TerminalColorMode::Auto) return static_cast<bool>(mode);
            return core::term::supports_color(handle);
        }

    private:
        writer_t m_writer;
        bool m_color_enabled{false};
        std::tuple<Style, Color, Color> m_current_style{
            default_style(), // style
            Color::Default, // text color
            Color::Default // bg color
        };
    };

    Terminal(
        FILE* handle,
        TerminalColorMode mode
    ) noexcept -> Terminal<FILE*>;
} // namespace dark

#endif // AMT_DARK_DIAGNOSTICS_CORE_TERMINAL_HPP
