#ifndef DARK_DIAGNOSTICS_CORE_STREAM_HPP
#define DARK_DIAGNOSTICS_CORE_STREAM_HPP

#include "terminal_config.hpp"
#include <functional>
#include <optional>
#include <sstream>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <variant>
#include <utility>

#ifdef DARK_LLVM_OS_STREAM
#include <llvm/Support/raw_ostream.h>
#endif

namespace dark {

    enum class StreamColorMode {
        Disable = 0,
        Enable,
        Auto,
    };

    struct StreamStyle {
        bool bg{false};
        bool bold{false};
        bool dim{false};
        bool strike{false};
        
        constexpr bool operator==(StreamStyle other) const noexcept {
            return other.bg == bg && other.strike == strike && other.dim == dim && other.bold == bold;
        }
        
        constexpr bool operator!=(StreamStyle other) const noexcept {
            return !(*this == other);
        }
    };

    namespace detail {
        template <typename T>
        struct is_printable_helper: std::false_type {};

        template <typename T>
        concept is_ostreamable = requires(T&& v) {
            { std::declval<std::ostringstream>() << v } -> std::same_as<std::ostringstream&&>;
        };

        #ifdef DARK_LLVM_OS_STREAM
            template <typename T>
            concept is_llvm_ostreamable = requires(T&& v) {
                { std::declval<llvm::raw_ostream>() << v } -> std::same_as<std::ostringstream&&>;
            };
        #endif

        template <typename T>
        concept is_printable = is_printable_helper<T>::value
            || is_ostreamable<T>
        #ifdef DARK_LLVM_OS_STREAM
            || is_llvm_ostreamable<T>
        #endif
        ;

    } // namespace detail

    class Stream {
        static constexpr std::size_t file_index = 0zu;
        #ifdef DARK_LLVM_OS_STREAM
            static constexpr std::size_t llvm_raw_stream_index = 1zu;
            static constexpr std::size_t ostream_index = 2zu;
        #else
            static constexpr std::size_t ostream_index = 1zu;
        #endif
    public:
        using base_type = std::variant<
            FILE*,
        #ifdef DARK_LLVM_OS_STREAM
            std::reference_wrapper<llvm::raw_ostream>,
        #endif
            std::reference_wrapper<std::ostream>
        >;
        #ifdef DARK_LLVM_OS_STREAM
            using Color = llvm::raw_ostream::Colors;
        #else
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
        #endif

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

        constexpr Stream(
            FILE* handle,
            StreamColorMode color_mode = StreamColorMode::Auto
        ) noexcept
            : m_data(handle)
            , m_color_enabled(supports_color(handle, color_mode))
        {}

        Stream(
            std::ostream& stream,
            StreamColorMode color_mode = StreamColorMode::Auto
        )   : m_data(stream)
            , m_color_enabled(supports_color(stream, color_mode))
        {}

        #ifdef DARK_LLVM_OS_STREAM
        Stream(
            llvm::raw_ostream& stream,
            StreamColorMode color_mode = StreamColorMode::Auto
        )   : m_data(stream)
            , m_color_enabled(supports_color(stream, color_mode))
        {
            stream.enable_colors(m_color_enabled);
        }
        #endif

        template <detail::is_printable T>
        auto operator<<(T&& val) -> Stream& {
            #ifdef DARK_LLVM_OS_STREAM
                if (m_data.index() == llvm_raw_stream_index) {
                    std::get<llvm_raw_stream_index>(m_data).get() << val;
                    return *this;
                }
            #endif
            if (m_data.index() == ostream_index) {
                auto& os = std::get<ostream_index>(m_data);
                os.get() << val;
            } else {
                auto handle = std::get<file_index>(m_data);
                if constexpr (std::convertible_to<T, std::string_view>) {
                    auto temp = std::string_view(val);
                    fprintf(handle, "%.*s", static_cast<int>(temp.size()), temp.data());
                } else {
                    std::string str;
                    auto ss = std::ostringstream();
                    ss << val;
                    str = ss.str();
                    fprintf(handle, "%.*s", static_cast<int>(str.size()), str.data());
                }
            }

            return *this;
        }

        auto flush() {
            #ifdef DARK_LLVM_OS_STREAM
                if (m_data.index() == llvm_raw_stream_index) {
                    std::get<llvm_raw_stream_index>(m_data).get().flush();
                    return;
                }
            #endif
            if (m_data.index() == file_index) {
                fflush(std::get<file_index>(m_data));
            } else {
                auto& os = std::get<ostream_index>(m_data);
                os.get().flush();
            }
        }

        constexpr auto is_displayed() const noexcept -> bool {
            #ifdef DARK_LLVM_OS_STREAM
                if (m_data.index() == llvm_raw_stream_index) return std::get<llvm_raw_stream_index>(m_data).get().is_displayed();
            #endif
            if (m_data.index() == ostream_index) return false;
            auto handle = std::get<file_index>(m_data);
            return core::internal::terminal_config.is_displayed(core::internal::terminal_config.get_fd_from_handle(handle));
        }

        auto prepare_colors() noexcept -> bool {
            if (!m_color_enabled) return false;
            if (core::internal::terminal_config.colors_need_flush() && !is_displayed()) return false;
            if (core::internal::terminal_config.colors_need_flush()) flush();
            return true;
        }

        auto change_color(Color color, StreamStyle style = {}) -> Stream& {
            // #ifdef DARK_LLVM_OS_STREAM
            //     if (m_data.index() == llvm_raw_stream_index) {
            //         std::get<llvm_raw_stream_index>(m_data).get().changeColor(color, style.bold, style.bg);
            //         return *this;
            //     }
            // #endif

            if (!prepare_colors()) return *this;
            core::internal::TerminalStyle s = {
                .bg = style.bg,
                .bold = style.bold,
                .dim = style.dim,
                .strike = style.strike,
            };
            if (color != Color::SAVEDCOLOR) s.code = static_cast<char>(color);
            auto colorcode = core::internal::terminal_config.output_color(s);
            this->operator<<(colorcode);
            return *this;
        }

        auto reset_color() -> Stream& {
            // #ifdef DARK_LLVM_OS_STREAM
            //     if (m_data.index() == llvm_raw_stream_index) {
            //         std::get<llvm_raw_stream_index>(m_data).get().resetColor();
            //         return *this;
            //     }
            // #endif
            if (!prepare_colors()) return *this;
            this->operator<<(core::internal::terminal_config.reset_color());
            return *this;
        }

        auto reverse_color() -> Stream& {
            // #ifdef DARK_LLVM_OS_STREAM
            //     if (m_data.index() == llvm_raw_stream_index) {
            //         std::get<llvm_raw_stream_index>(m_data).get().reverseColor();
            //         return *this;
            //     }
            // #endif
            if (!prepare_colors()) return *this;
            this->operator<<(core::internal::terminal_config.output_reverse());
            return *this;
        }

        constexpr auto enable_colors(bool enable) {
            m_color_enabled = enable;

            #ifdef DARK_LLVM_OS_STREAM
                if (m_data.index() == llvm_raw_stream_index) {
                    std::get<llvm_raw_stream_index>(m_data).get().enable_colors(enable);
                    return;
                }
            #endif
        }

    private:
        static auto supports_color(FILE* handle, StreamColorMode color_mode) noexcept -> bool {
            if (color_mode != StreamColorMode::Auto) return static_cast<bool>(color_mode);
            return core::internal::terminal_config.supports_color(core::internal::terminal_config.get_fd_from_handle(handle));
        }

        static auto supports_color(std::ostream&, StreamColorMode color_mode) noexcept -> bool {
            if (color_mode != StreamColorMode::Auto) return static_cast<bool>(color_mode);
            return false;
        }

        #ifdef DARK_LLVM_OS_STREAM
        static auto supports_color(llvm::raw_ostream& os, StreamColorMode color_mode) noexcept -> bool {
            if (color_mode != StreamColorMode::Auto) return static_cast<bool>(color_mode);
            return os.has_colors();
        }
        #endif

    private:
        base_type m_data;
        bool m_color_enabled{false};
    };

    static inline Stream& out() noexcept {
        #ifdef DARK_LLVM_OS_STREAM
            static auto stream = Stream(llvm::out());
        #else
            static auto stream = Stream(stdout);
        #endif
        return stream;
    }

    static inline Stream& err() noexcept {
        #ifdef DARK_LLVM_OS_STREAM
            static auto stream = Stream(llvm::err());
        #else
            static auto stream = Stream(stderr);
        #endif
        return stream;
    }
} // namespace dark

inline auto operator<<(dark::Stream& os, dark::Stream::Color const& c) -> dark::Stream& {
    return os.change_color(c);
}

#endif // DARK_DIAGNOSTICS_CORE_STREAM_HPP
