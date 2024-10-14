#ifndef DARK_DIAGNOSTICS_PRINTER_HPP
#define DARK_DIAGNOSTICS_PRINTER_HPP

#include "diagnostics/basic.hpp"
#include "diagnostics/core/cow_string.hpp"
#include "diagnostics/core/formatter.hpp"
#include "diagnostics/core/small_vector.hpp"
#include "diagnostics/core/stream.hpp"
#include "diagnostics/core/string_utils.hpp"
#include "diagnostics/core/terminal_config.hpp"
#include "diagnostics/core/utf8.hpp"
#include "diagnostics/span.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <map>
#include <array>
#include <unordered_set>
#include <utility>
#include <vector>

#ifndef DARK_MIN_TERMINAL_COLS
    #define DARK_MIN_TERMINAL_COLS 100
#endif

namespace dark::internal {
    static constexpr auto line_color = Stream::Color::BLUE;
    
    enum class MarkerKind: std::uint8_t {
        Note    = 1,
        Remark,
        Warning,
        Error,
        Remove,
        Insert,
        Size,
    };

    static constexpr auto marker_kind_to_string(MarkerKind kind) noexcept -> std::string_view {
        switch (kind) {
        case MarkerKind::Note: return "Note";
        case MarkerKind::Remark: return "Remark";
        case MarkerKind::Warning: return "Warning";
        case MarkerKind::Error: return "Error";
        case MarkerKind::Remove: return "Remove";
        case MarkerKind::Insert: return "Insert";
        default: std::unreachable();
        }
    }

    static constexpr auto to_marker_kind(DiagnosticLevel level) noexcept -> MarkerKind {
        switch (level) {
        case dark::DiagnosticLevel::Note: return MarkerKind::Note; 
        case dark::DiagnosticLevel::Remark: return MarkerKind::Remark;
        case dark::DiagnosticLevel::Warning: return MarkerKind::Warning;
        case dark::DiagnosticLevel::Error: return MarkerKind::Error;
        }
    } 
    
    static constexpr auto to_marker_kind(DiagnosticOperationKind kind) noexcept -> MarkerKind {
        switch (kind) {
        case dark::DiagnosticOperationKind::Insert: return MarkerKind::Insert;
        case dark::DiagnosticOperationKind::Remove: return MarkerKind::Remove;
        default: return MarkerKind::Error;
        }
    } 

    static constexpr auto to_marker_kind(BasicDiagnosticMessage const& m) noexcept -> MarkerKind {
        if (m.op != DiagnosticOperationKind::None) return to_marker_kind(m.op);
        return to_marker_kind(m.level);
    } 


    static constexpr inline auto get_z_index(MarkerKind kind, std::uint8_t base = 0) noexcept -> std::uint8_t {
        return ( static_cast<std::uint8_t>(kind) + base );
    }

    static constexpr inline auto get_z_index(BasicDiagnosticMessage const& message, std::uint8_t base = 0) noexcept -> std::uint8_t {
        return get_z_index(to_marker_kind(message), base);
    }

    static constexpr inline auto get_color(MarkerKind kind) noexcept -> Stream::Color {
        switch (kind) {
        case MarkerKind::Note: return diagnostic_level_to_color(DiagnosticLevel::Note);
        case MarkerKind::Remark: return diagnostic_level_to_color(DiagnosticLevel::Remark);
        case MarkerKind::Warning: return diagnostic_level_to_color(DiagnosticLevel::Warning);
        case MarkerKind::Error: return diagnostic_level_to_color(DiagnosticLevel::Error);
        case MarkerKind::Remove: return Stream::Color::RED;
        case MarkerKind::Insert: return Stream::Color::GREEN;
        default: std::unreachable();
        }
    }

    static constexpr inline auto get_color(BasicDiagnosticMessage const& message) noexcept -> Stream::Color {
        return get_color(to_marker_kind(message));    
    }

    
    static constexpr inline auto get_marker_char(MarkerKind kind) noexcept -> char {
        switch (kind) {
            case MarkerKind::Insert: return '+';
            case MarkerKind::Remove: return '-';
            default: return '~';
        }
    }

    static constexpr inline auto get_marker_char(BasicDiagnosticMessage const& message) noexcept -> char {
        return get_marker_char(to_marker_kind(message));
    }

    static constexpr inline auto get_z_index(DiagnosticLevel level) noexcept -> std::uint8_t {
        return get_z_index(to_marker_kind(level)); 
    }
    
    static constexpr inline auto get_color(DiagnosticLevel level) noexcept -> Stream::Color {
        return diagnostic_level_to_color(level);
    }
    
    static constexpr inline auto get_marker_char(DiagnosticLevel level) noexcept -> char {
        return get_marker_char(to_marker_kind(level));
    }


    enum class MarkerArrowDirection {
        None,
        Up,     // "v"
        Down,   // "^"
        Left,   // "->"
        Right,  // "<-"
        Size
    };

    constexpr auto marker_arrow_to_string(MarkerArrowDirection dir) noexcept {
        switch (dir) {
            case MarkerArrowDirection::None: return "None";
            case MarkerArrowDirection::Up: return "Up";
            case MarkerArrowDirection::Down: return "Down";
            case MarkerArrowDirection::Left: return "Left";
            case MarkerArrowDirection::Right: return "Right";
            case MarkerArrowDirection::Size: return "Size";
        }
    };

    struct BoundingBox {
        std::size_t min_x{};
        std::size_t min_y{};
        std::size_t max_x{};
        std::size_t max_y{};
        
        friend auto& operator<<(std::ostream& os, BoundingBox const& box) {
            os << "Box(minX: " << box.min_x <<", minY: " << box.min_y << ", maxX: " << box.max_x << ", maxY: " << box.max_y << ")\n";
            return os;
        }
    };

    struct CursorPosition {
        std::size_t row;
        std::size_t col;

        constexpr auto next_line() noexcept -> void {
            row = (col == 0 ? 0zu : 1zu) + row;
            col = 0;
        }

        constexpr auto go(MarkerArrowDirection dir, BoundingBox box) const noexcept -> CursorPosition {
            switch (dir) {
            case MarkerArrowDirection::Down: return { .row = std::min(box.max_y, row + 1), .col = col };
            case MarkerArrowDirection::Up: return { .row = std::max(std::max(row, 1zu) - 1zu, box.min_y), .col = col };
            case MarkerArrowDirection::Left: return { .row = row, .col = std::max(std::max(col, 1zu) - 1zu, box.min_x) };
            case MarkerArrowDirection::Right: return { .row = row, .col = std::min(box.max_x, col + 1)};
            default: return *this;
            }
        }

        friend std::ostream& operator<<(std::ostream& os, CursorPosition pos)  {
            os << "Cursor(row: " << pos.row << ", col: " << pos.col << ")";
            return os;
        }

        constexpr auto to_pair() const noexcept -> std::pair<std::size_t, std::size_t> {
            return { row, col };
        }

        constexpr auto operator==(CursorPosition pos) const noexcept -> bool {
            return to_pair() == pos.to_pair();
        }

        constexpr auto operator!=(CursorPosition pos) const noexcept -> bool {
            return !(*this == pos);
        }

        constexpr auto is_inside(BoundingBox box) const noexcept -> bool {
            if (box.min_x < col || box.max_x <= col) return false;
            if (box.min_y < row || box.max_y <= row) return false;
            return true;
        }

        [[nodiscard("This function does not mutate")]] constexpr auto clamp(BoundingBox box) const noexcept -> CursorPosition {
            return { std::max(std::min(box.max_y, row), box.min_y), std::max(std::min(box.max_x, col), box.min_x) };
        };
    };

    struct CursorPositionHash {
        constexpr auto operator()(CursorPosition c) const noexcept -> std::size_t {
            auto h1 = std::hash<std::size_t>{}(c.row);
            auto h2 = std::hash<std::size_t>{}(c.col);
            
            return h1 ^ (h2 << 1);
        } 
    };

    struct DisplaySpan {
        CursorPosition cursor{};
        std::size_t context_index{};
        std::size_t item_index{};
    };
    

    struct Style {
        std::optional<Stream::Color> color {};
        bool is_bg{false};
        bool is_bold{false};
        bool is_dim{false};
        bool is_strike{false};
        std::uint8_t z{};
        unsigned start_padding{};
        unsigned end_padding{};
        bool keep_ident{};
        bool prevent_wordbreak{};

        std::optional<unsigned> id{};

        [[nodiscard("This function does not mutate.")]] constexpr auto bold() const noexcept -> Style {
            auto temp = *this;
            temp.is_bold = true;
            return temp;
        }
        
        [[nodiscard("This function does not mutate.")]] constexpr auto normal() const noexcept -> Style {
            auto temp = *this;
            temp.is_bold = false;
            return temp;
        }

        constexpr auto to_stream_color() const noexcept -> StreamStyle {
            return {
                .bg = is_bg,
                .bold = is_bold,
                .dim = is_dim,
                .strike = is_strike
            };
        }
    };

    struct TerminalScreen {
        struct Cell {
            char c[4] = {0};
            std::uint8_t len{};
            Style style{};

            constexpr static auto from_utf8(
                std::string_view s,
                Style style = {}
            ) noexcept -> Cell {
                auto cell = Cell();
                cell.len = s.empty() ? 0 : ::dark::core::utf8::get_length(s[0]);
                cell.style = style;
                auto const slen = static_cast<std::size_t>(cell.len);

                for (auto i = 0zu; i < slen; ++i) {
                    cell.c[i] = s[i];
                }

                return cell;
            }
            
            constexpr auto to_view() const noexcept -> std::string_view {
                return { c, static_cast<std::size_t>(len) };
            }

            constexpr auto empty() const noexcept -> bool {
                return len == 0;
            }

            constexpr auto empty_or_space() const noexcept -> bool {
                return empty() || to_view() == " ";
            }
        };

        TerminalScreen(
            std::size_t rows = 100zu,
            std::size_t cols = std::max(core::internal::terminal_config.get_columns(STDOUT_FILENO), static_cast<std::size_t>(DARK_MIN_TERMINAL_COLS))
        ) noexcept
            : m_rows(rows)
            , m_cols(cols)
            , m_data(m_rows * m_cols)
        {}

        TerminalScreen(TerminalScreen const&) = default;
        TerminalScreen& operator=(TerminalScreen const&) = default;
        constexpr TerminalScreen(TerminalScreen&&) noexcept = default;
        TerminalScreen& operator=(TerminalScreen&&) noexcept = default;
        ~TerminalScreen() = default;

        constexpr auto operator()(std::size_t row, std::size_t col) noexcept -> Cell& {
            return m_data[row * m_cols + col];
        }

        constexpr auto operator()(std::size_t row, std::size_t col) const noexcept -> Cell const& {
            return m_data[row * m_cols + col];
        }
        
        constexpr auto operator()(CursorPosition c) noexcept -> Cell& {
            return this->operator()(c.row, c.col); 
        }

        constexpr auto operator()(CursorPosition c) const noexcept -> Cell const& {
            return this->operator()(c.row, c.col); 
        }


        constexpr auto cols() const noexcept -> std::size_t {
            return m_cols;
        }
        
        constexpr auto rows() const noexcept -> std::size_t {
            return m_rows;
        }

        constexpr auto can_insert_between_cols(std::size_t row, std::size_t start, std::size_t end) const noexcept -> bool {
            end = std::min(end, cols());
            for (; start < end; ++start) {
                auto& cell = this->operator()(row, start);
                if (!cell.empty_or_space()) return false;
            }
            return true;
        }

        constexpr auto try_fix_row(std::size_t row) noexcept {
            auto col = cal_row_size(row);
            for (auto i = 0zu; i < col; ++i) {
                auto& cell = this->operator()(row, i);
                if (!cell.empty()) continue;
                cell = Cell::from_utf8(" ");
            }
        
        }

        [[nodiscard]] constexpr auto get_rows_to_print() const noexcept -> std::size_t {
            for (auto i = 0zu; i < m_rows; ++i) {
                auto c = 0zu;
                for (auto j = 0zu; j < m_cols; ++j) {
                    auto const& cell = m_data[i * m_cols + j];
                    c += (cell.empty() ? 1 : 0);
                }
                if (c == m_cols) {
                    return i;
                }
            }

            return 0zu;
        }

        [[maybe_unused]] constexpr auto cal_row_size(std::size_t row_idx) const noexcept -> std::size_t {
            auto* row = &m_data[row_idx * m_cols];
            std::size_t cols{};
            for (auto i = 0zu; i < m_cols; ++i) {
                Cell const& cell = row[i]; 
                if (cell.empty()) continue;
                if (cell.len == 1 && cell.c[0] == ' ') continue;
                cols = i + 1;
            }
            return cols;
        }


        friend auto operator<<(Stream& os, TerminalScreen const& t) -> Stream& {
            auto max_rows = t.get_rows_to_print();
            auto last_color = std::optional<Stream::Color>{};
            auto last_style = StreamStyle { };
            
            for (auto i = 0zu; i < max_rows; ++i) {
                auto cols = t.cal_row_size(i);
                for (auto j = 0zu; j < cols; ++j) {
                    auto const cell = t(i, j);
                    auto text = cell.to_view();
                    if (cell.empty()) text = " ";
                    auto style = cell.style.to_stream_color();
                    if (cell.style.color != last_color) {
                        if (cell.style.color) {
                            os.change_color(*cell.style.color, style);
                            last_color = cell.style.color;
                            last_style = style;
                        }
                        else {
                            os.reset_color();
                            last_color = std::nullopt;
                            last_style = {};
                        }
                    }

                    if (style != last_style) {
                        os.change_color(last_color.value_or(Stream::SAVEDCOLOR), style);
                        last_style = style;
                    }

                    os << text;
                }
                os << '\n';
            }

            os.reset_color();
            return os << '\n';
        }

        auto append_rows(
            std::size_t num_rows = 1,
            std::string_view s = "",
            Style style = {}
        ) {
            if (num_rows == 0) return;

            auto cell = Cell::from_utf8(s, style);
            m_data.resize(m_data.size() + m_cols * num_rows, cell);
            m_rows += num_rows;
        }

        auto insert_row(
            std::size_t pos
        ) {
            if (pos > m_rows) return;
            if (pos == m_rows) append_rows();
            
            auto last_row = get_rows_to_print();
            if (last_row == m_rows) append_rows();
            
           for (auto i = last_row; i > pos; --i) {
                for (auto j = 0zu; j < m_cols; ++j) {
                   this->operator()(i, j)  = std::move(this->operator()((i - 1), j));
                }
            }
            
            for (auto j = 0zu; j < m_cols; ++j) {
                this->operator()(pos, j) = {};
            }
            
            this->operator()(pos, 0) = Cell::from_utf8(" ");
        }

        constexpr auto get_current_row() const noexcept -> std::span<Cell const> {
            auto& self = *const_cast<TerminalScreen*>(this);
            return self.get_current_row();
        }

        constexpr auto get_current_row() noexcept -> std::span<Cell> {
            if (m_current >= m_rows) return {};
            return {
                m_data.begin() + static_cast<std::ptrdiff_t>(m_current * m_cols),
                m_cols
            };
        }

        constexpr auto next_row() noexcept -> void {
            ++m_current;
        }

        constexpr auto write(std::string_view text, CursorPosition& cursor, Style style, auto&& on_new_line) {
            auto padding = cursor.col;
            auto orginal_start_padding = style.start_padding;
            bool is_padding_set{false};
            while (true) {
                auto n = try_write(text, cursor, style);
                if (n == text.size()) break;
                cursor = {
                    .row = cursor.row + 1,
                    .col = 0
                };
                if (!is_padding_set) {
                    style.start_padding = orginal_start_padding + (style.keep_ident ? static_cast<unsigned>(padding) : 0);
                    is_padding_set = true;
                }
                on_new_line(cursor, style);
                text = core::utils::trim(text.substr(n));
            }
            style.start_padding = orginal_start_padding;
        }

        constexpr auto write(std::string_view text, CursorPosition& cursor, Style style = {}) {
            write(text, cursor, style, [](auto&, auto&) {});
        }

        constexpr auto pad_with(std::string_view text, CursorPosition& cursor, unsigned padding, Style style = {}) noexcept {
            while (padding--) {
                write(text, cursor, style);
            }
        }

        constexpr auto try_write(std::string_view text, CursorPosition& cursor, Style style = {}) -> std::size_t {
            if (text.empty()) return 0;

            pad_with(" ", cursor, style.start_padding);
            auto const end_padding = static_cast<std::size_t>(style.end_padding);

            auto [row, col] = cursor;

            if (col >=m_cols) {
                ++row;
                col = 0;
            }

            ensure_rows(row);
            auto left_space = std::max(end_padding, m_cols - col) - style.end_padding;
            if (style.prevent_wordbreak) {
                text = remove_wordbreak(text, left_space);
            }
            auto size = text.size();
            std::size_t written{0};

            for (auto j = 0zu; j < std::min(size, left_space);) {
                auto& old_cell = this->operator()(row, col + j);
                ++written;
                if (old_cell.style.z > style.z) {
                    j += core::utf8::get_length(text[j]);
                    continue;
                }
                if (text[j] == '\n') {
                    text = text.substr(j + 1);
                    j = 0;
                    ++row;
                    col = 0;
                    ensure_rows(row);
                    left_space = std::max(end_padding, m_cols - col) - style.end_padding;
                    size = text.size();
                    continue;
                } else if (text[j] == '\r') {
                    text = text.substr(j + 1);
                    j = 0;
                    col = 0;
                    left_space = std::max(end_padding, m_cols - col) - style.end_padding;
                    size = text.size();
                    continue;
                }
                old_cell = Cell::from_utf8(text.substr(j), style);
                j += old_cell.len;
            }

            if (written >= size) {
                col += size;
                cursor = { .row = row, .col = col };
                return written;
            }
            cursor = { .row = row, .col = col };
            return written;
        }

        constexpr auto write_escape(std::string_view s, CursorPosition& cursor, Style style = {}) -> void {
            auto i = 0zu;
            auto const size = s.size();
            while (i < size) {
                i += escape_char(s, i, [this, &cursor, style](std::string_view s){
                    write(s, cursor, style);
                });
            }
        }
        constexpr auto try_write_escape(std::string_view s, CursorPosition& cursor, Style style = {}) -> std::size_t {
            auto i = 0zu;
            auto const size = s.size();
            while (i < size) {
                bool has_written = true;
                std::size_t written_len{i};
                i += escape_char(s, i, [this, &has_written, &written_len, &cursor, style](std::string_view s){
                    auto const n = try_write(s, cursor, style);
                    if (n != s.size()) {
                        has_written = false;
                    } else {
                        has_written = true;
                    }
                    written_len += n;
                });
                
                if (!has_written) return written_len;
            }
            return size;
        }
    private:
        constexpr auto ensure_rows(std::size_t row) -> void {
            while (row >= m_rows) append_rows();
        }

        constexpr auto remove_wordbreak(std::string_view text, std::size_t width) const noexcept -> std::string_view {
            if (text.size() <= width) return text;
            text = text.substr(0, width);
            auto new_line = text.find_first_of("\n");
            if (new_line < text.size()) {
                text = text.substr(0, new_line + 1);
            }
            auto last_space = text.find_last_of(" ");
            
            // we cannot prevent word break
            if (last_space >= text.size()) return text;
            return text.substr(0, last_space);
        }
        
        constexpr auto escape_char(std::string_view s, std::size_t i, auto&& fn) -> std::size_t {
            auto len = static_cast<std::size_t>(core::utf8::get_length(s[i]));
            auto temp = s.substr(i, len);
            bool is_escape{false};
            char buff[4] = {'\\', 0, 0, 0};
            switch (temp[0]) {
                case '\n': {
                    buff[1] = 'n';
                    buff[2] = 0;
                    is_escape = true;
                    break;
                }
                case '\t': {
                    buff[1] = 't';
                    buff[2] = 0;
                    is_escape = true;
                    break;
                }
                case '\b': {
                    buff[1] = 'b';
                    buff[2] = 0;
                    is_escape = true;
                    break;
                }
                case '\a': {
                    buff[1] = 'a';
                    buff[2] = 0;
                    is_escape = true;
                    break;
                }
                case '\f': {
                    buff[1] = 'f';
                    buff[2] = 0;
                    is_escape = true;
                    break;
                }
                case '\v': {
                    buff[1] = 'v';
                    buff[2] = 0;
                    is_escape = true;
                    break;
                }
                default: break;
            }
            if (is_escape) {
                fn(std::string_view(buff));
                return 1zu;
            } else { 
                fn(temp); 
                return temp.size();
            }
        }
    private:
        std::size_t m_rows{};
        std::size_t m_cols{};
        std::size_t m_current{};
        std::vector<Cell> m_data;
    };


    struct MessageIdPair {
        std::size_t message_id{};
        std::size_t span_id{};
        
        constexpr auto operator==(MessageIdPair const& other) const noexcept -> bool {
            return (message_id == other.message_id) && (span_id == other.span_id);
        }
        
        constexpr auto operator!=(MessageIdPair const& other) const noexcept -> bool {
            return !(*this == other); 
        }
    
        template <std::size_t N>
        constexpr auto has_same_marker(core::SmallVec<BasicDiagnosticMessage, N> const& context, MessageIdPair other) {
            auto const& lm = context[message_id];
            auto const& rm = context[other.message_id];
            
            return get_z_index(lm) == get_z_index(rm);
        }
    };


    struct DiagnosticRenderItemInfo {
        core::CowString text{};
        std::size_t start{};
        core::SmallVec<MessageIdPair, 5> ids{};
        Style style{};
        std::optional<MarkerKind> kind{};

        constexpr auto is_skippable() const noexcept -> bool {
            return !kind.has_value();
        }
    };

    struct DiagnosticRenderLineInfo {
        std::vector<DiagnosticRenderItemInfo> infos;
        unsigned line_number{};

    private:
            struct MaskItem {
                bool is_used{false};
                std::uint8_t z_index{};
                core::SmallVec<MessageIdPair, 5> message_ids;
                std::size_t id{};
            };

    public:

        static auto parse(
            Span parent_span,
            unsigned line_number,
            core::SmallVec<BasicDiagnosticMessage, 2> const& messages,
            auto&& iterate_messages,
            auto&& set_token_info
        ) -> DiagnosticRenderLineInfo {
            auto res = DiagnosticRenderLineInfo {};
            res.line_number = line_number;

            // 1. Store the marked block that are referenced            
            std::vector<MaskItem> used_chars(parent_span.size());

            auto set_used_chars = [&used_chars, id = 0zu](
                Span span,
                std::uint8_t z,
                std::optional<std::size_t> m_id,
                std::size_t s_id
            ) mutable {
                auto new_id = ++id;
                for (auto i = span.start(); i < span.end(); ++i) {
                    auto el = used_chars[i];
                    auto old_ids = std::move(used_chars[i].message_ids);
                    if (m_id) old_ids.push_back(MessageIdPair{ *m_id, s_id });
                    
                    used_chars[i] = {
                        .is_used = true,
                        .z_index = std::max(el.z_index, z),
                        .message_ids = std::move(old_ids),
                        .id = (el.z_index <= z ? new_id : el.id)
                    };
                }
            };
            
            std::vector<std::pair<std::size_t, std::size_t>> inserts{};


            iterate_messages([&](Span range, unsigned absolute_pos){
                auto abs_pos = static_cast<ptrdiff_t>(absolute_pos);
                for (auto i = 0zu; i < messages.size(); ++i) {
                    auto const& message = messages[i];
                    for (auto j = 0zu; j < message.spans.size(); ++j) {
                        auto span = message.spans[j];
                        if (!span.intersects(range)) continue;

                        if (message.op == DiagnosticOperationKind::Insert) {
                            inserts.push_back({ i, j });                            
                            continue;
                        }
                        set_used_chars(
                            span.shift(-abs_pos).clip_end(parent_span.size()),
                            get_z_index(message),
                            i,
                            j
                        );
                    }
                }
            });

            // 1.5. Sort the mask items based on z-indices
            
            for (auto& el: used_chars) {
                std::stable_sort(el.message_ids.begin(), el.message_ids.end(), [&messages](auto l, auto r){
                    return get_z_index(messages[l.message_id]) > get_z_index(messages[r.message_id]);
                });
            }
            
            
            // 2. Build the marked word blocks
            
            constexpr auto insert_message_ids = []<std::size_t N>(core::SmallVec<MessageIdPair, N>& res, core::SmallVec<MessageIdPair, N> in) {
                for (auto& el : in) {
                    auto found = false;
                    for (auto const& out: res) {
                        if (el == out) {
                            found = true;
                            break;
                        } 
                    }
                    if (!found) {
                        res.push_back(std::move(el));
                    }
                }
            };

            auto const sort_message_ids = [&messages]<std::size_t N>(core::SmallVec<MessageIdPair, N>& res) {
                std::stable_sort(res.begin(), res.end(), [&messages](auto const& l, auto const& r) {
                    auto lm = messages[l.message_id];
                    auto ls = lm.spans[l.span_id];
                    auto rm = messages[r.message_id];
                    auto rs = rm.spans[r.span_id];
                    if (ls.start() == rs.start()) return get_z_index(lm) >= get_z_index(rm);
                    return ls.start() < rs.start();
                });
            };

        
            // open-ended range [start, end)
            auto const update_info = [&](std::size_t start, std::size_t end) {
                if (start >= end) return;
                auto& item = used_chars[start];
                auto is_marked = item.is_used && !item.message_ids.empty();

                auto message_ids = core::SmallVec<MessageIdPair, 5>();
                for (auto i = start; i < end; ++i) {
                    auto& el = used_chars[i];
                    insert_message_ids(message_ids, std::move(el.message_ids));
                }

                sort_message_ids(message_ids);
                
                set_token_info(start, end, item, is_marked, [&](core::CowString text, Style style, bool marked){
                    if (marked) {
                        auto const& message = messages[item.message_ids[0].message_id];
                        if (message.op == DiagnosticOperationKind::Remove) {
                            style.is_bold = false;
                            style.is_dim = true;
                            style.is_strike = true;
                        }
                        auto info = DiagnosticRenderItemInfo {
                            .text = std::move(text),
                            .start = start,
                            .ids = message_ids,
                            .style = style,
                            .kind = to_marker_kind(message)
                        };
                        res.infos.push_back(std::move(info));
                    } else {
                        auto info = DiagnosticRenderItemInfo {
                            .text = std::move(text),
                            .start = start,
                            .ids = message_ids,
                            .style = style 
                        };
                        res.infos.push_back(std::move(info));
                    } 
                });
            };


            {            
                auto last_pos = 0zu;
                auto i = 0zu;
                auto const size = used_chars.size();

                while (i < size) {
                    auto current = used_chars[i];
                    auto last = used_chars[last_pos];
                    if (current.is_used != last.is_used || current.id != last.id) {
                        update_info(last_pos, i);
                        last_pos = i;
                    }
                    ++i;
                }
                
                if (last_pos < size) {
                    update_info(last_pos, size);
                }
            }

            // 3. Insert the text
            auto const abs_pos = static_cast<std::ptrdiff_t>(parent_span.start());
            for (auto [m_id, s_id]: inserts) {
                auto len = 0zu;
                auto const& m = messages[m_id];
                auto span = m.spans[s_id].shift(-abs_pos);
                auto start = static_cast<std::size_t>(span.start());
                
                auto it = res.infos.begin();
                for (; it != res.infos.end(); ++it) {
                    auto const& item = *it;
                    auto text = item.text;

                    if (!item.ids.empty()) {
                        auto const& temp = messages[item.ids[0].message_id];
                        if (temp.op == DiagnosticOperationKind::Insert) continue;
                    }
                    
                    if (len >= start) {
                        break;
                    }   

                    len += text.size();
                }
                
                if (it == res.infos.end()) continue;
                auto const& item = *it;
                
                
                if (len == start) {
                    auto info = DiagnosticRenderItemInfo {
                        .text = m.text_to_be_inserted.to_borrowed(),
                        .start = start,
                        .style = { .color = get_color(m), .is_bold = true, .z = get_z_index(m) },
                        .kind = MarkerKind::Insert
                    };
                    info.ids.push_back({ .message_id = m_id, .span_id = s_id });
                    res.infos.insert(it, std::move(info));
                } else {
                    auto size = len - start;
                    auto f = item;
                    f.text = item.text.substr(0, size);
                    auto mid = DiagnosticRenderItemInfo {
                        .text = m.text_to_be_inserted.to_borrowed(),
                        .start = start,
                        .style = { .color = get_color(m), .is_bold = true, .z = get_z_index(m) },
                        .kind = MarkerKind::Insert
                    };
                    mid.ids.push_back({ .message_id = m_id, .span_id = s_id });

                    auto l = item;
                    l.text = item.text.substr(size);
                    
                    if (f.text.empty()) {
                        *it = mid;
                        res.infos.insert(it, { l });
                    } else {
                        *it = f;
                        if (l.text.empty()) res.infos.insert(it + 1, { mid });
                        else res.infos.insert(it + 1, { mid, l }); 
                    }
                }
            }

            return res;
        }
        
        static auto parse(
            std::string_view source,
            unsigned line_number,
            Span parent_span,
            core::SmallVec<BasicDiagnosticMessage, 2> const& messages
        ) -> DiagnosticRenderLineInfo {
            return parse(
                parent_span,
                line_number,
                messages,
                [parent_span](auto&& body) {
                    body(parent_span, parent_span.start());
                },
                [&](std::size_t start, std::size_t end, MaskItem const& item, bool is_marked, auto&& push_back) {
                    auto text = source.substr(start, end - start);
                    if (is_marked) {
                        auto const& message = messages[item.message_ids[0].message_id];
                        push_back(
                            text,
                            Style { .color = get_color(message), .is_bold = true, .z = get_z_index(message)  },
                            is_marked
                        );
                    } else {
                        push_back(text, {}, is_marked);
                    }
                }

            );
        }


        
        static auto parse(
            LineDiagnosticToken& line,
            core::SmallVec<BasicDiagnosticMessage, 2> const& messages
        ) -> DiagnosticRenderLineInfo {
            auto const source_span = line.span();
            auto const line_number = line.line_number;
            auto& tokens = line.tokens;

            auto const push_token_info = [&tokens, line, &source_span](std::size_t start, std::size_t end, auto&& push_back) {
                auto span = LocRelSpan::from_usize(static_cast<unsigned>(start), end - start)
                    .resolve(source_span.start())
                    .clip_start(source_span.start())
                    .clip_end(source_span.end());

                auto start_range_index = tokens.size();

                if (!span.intersects(source_span)) return;
                
                for (auto i = 0zu; i < tokens.size(); ++i) {
                    if (span.intersects(line.span_for(i))) {
                        start_range_index = i;
                        break;
                    }
                }

                for (; start_range_index < tokens.size(); ++start_range_index) {
                    auto const& item = tokens[start_range_index];
                    auto token_span = item.span().resolve(line.source_location);
                    if (!span.intersects(token_span)) break;

                    auto start_off = token_span.start();
                    
                    if (span.start() <= token_span.start()) {
                        auto l = token_span.start() - span.start();
                        if (l != 0) {
                            push_back(std::string(l, ' '), start_range_index);
                            span = Span(span.start() + l, span.end());
                        }
                    } else {
                        start_off = std::max(start_off, span.start());
                    }

                    start_off -= token_span.start();
                     
                    if (span.end() >= token_span.end()) {
                        auto text = item.token.substr(start_off);
                        if (text.empty()) continue;
                        push_back(text, start_range_index);
                        span = Span(token_span.end(), span.end());
                        continue;
                    }
                    auto temp = Span(std::max(token_span.start(), span.start()), std::min(token_span.end(), span.end()));
                    auto text = item.token.substr(start_off, temp.size());
                    push_back(text, start_range_index);
                    return;
                }

                if (span.empty()) return;
                push_back(std::string(span.size(), ' '), start_range_index);
            };

            return parse(
                source_span,
                line_number,
                messages,
                [&tokens, line, source_span](auto&& body) {
                    for (auto i = 0zu; i < tokens.size(); ++i) {
                        body(line.span_for(i), source_span.start());
                    }
                },
                [&](std::size_t start, std::size_t end, [[maybe_unused]] MaskItem const& item, bool is_marked, auto&& push_back) {
                    push_token_info(start, end, [&](core::CowString text, std::size_t index){
                        auto style = Style { .z = 100 };
                        if (index < tokens.size()) {
                            auto const& token = tokens[index];
                            style = Style { .color = token.color, .is_bold = token.is_bold, .z = 100 };
                        }
                        push_back(std::move(text), style, is_marked);
                    });
                }
            );
        }


        constexpr auto is_skippable() const noexcept -> bool {
            auto count {0zu};
            for (auto const& el: infos) {
                if (el.is_skippable()) ++count;
            }

            return count == infos.size();
        }
    };

    struct DiagnosticRenderContext {
        core::SmallVec<DiagnosticRenderLineInfo, 2> lines;

        static auto from_loaction_item(
            BasicDiagnosticLocationItem const& loc,
            core::SmallVec<BasicDiagnosticMessage, 2> const& messages,
            std::size_t skip_line_cutoff = 5 // 0 means no line will be removed
        ) -> DiagnosticRenderContext {
            DiagnosticRenderContext res{};

            auto last_pos = 0zu;
            auto abs_pos = loc.source_location;
            auto line_number = loc.line_number;
            
            while (last_pos < loc.source.size()) {
                auto it = loc.source.find_first_of('\n', last_pos);
                auto text = loc.source;

                if (it >= loc.source.size()) {
                    if (last_pos == it) break;
                    text = loc.source.substr(last_pos);
                    last_pos = it;
                } else {
                    text = loc.source.substr(last_pos, (it - last_pos));
                    last_pos = it + 1;
                }
                
                auto span = Span::from_usize(abs_pos, text.size());
                auto line_info = DiagnosticRenderLineInfo::parse(text, line_number, span,  messages);
                res.lines.push_back(std::move(line_info));
                abs_pos += text.size();
                ++line_number;
            }

            if (skip_line_cutoff) {
                auto start = res.lines.begin();
                while (!res.lines.empty()) {
                    for (auto i = start; i != res.lines.end(); ++i) {
                        if (i->is_skippable()) {
                            start = i;
                            break;
                        }
                    }
                    
                    auto end = start;
                    for (auto i = start; i != res.lines.end(); ++i) {
                        if (i->is_skippable()) {
                            end = i;
                            break;
                        }
                    }
                    
                    auto dist = std::distance(start, end);
                    if (dist >= static_cast<std::ptrdiff_t>(skip_line_cutoff)) {
                        res.lines.erase(start, end);
                    } else break;

                    start = res.lines.begin();
                }
            }
            
            return res;
        }

        static auto from_tokens(
            DiagnosticLocationTokens& loc,
            core::SmallVec<BasicDiagnosticMessage, 2>& messages
        ) -> DiagnosticRenderContext {
            auto res = DiagnosticRenderContext{};

            std::stable_sort(loc.lines.begin(), loc.lines.end(), [](auto const& l, auto const& r) {
                return l.line_number < r.line_number;
            });

            for (auto& line: loc.lines) {
                std::stable_sort(line.tokens.begin(), line.tokens.end(), [](auto const& l, auto const& r){
                    return l.column_number < r.column_number; 
                });
                auto line_info = DiagnosticRenderLineInfo::parse(line, messages);
                res.lines.push_back(std::move(line_info));
            } 

            return res;
        }
    };


    static constexpr inline auto render_error(
        TerminalScreen& screen,
        CursorPosition& cursor,
        DiagnosticLevel level,
        core::Formatter&& formatter
    ) noexcept -> void {
        screen.write(diagnostic_level_to_string(level), cursor, { .color = diagnostic_level_to_color(level), .is_bold = true });
        screen.write(": ", cursor, { .color = diagnostic_level_to_color(level), .is_bold = true });
        auto format = formatter.format();
        screen.write(format, cursor, { .is_bold = true, .keep_ident = true, .prevent_wordbreak = true });
        cursor.next_line();
    }

    static constexpr inline auto get_max_line_number_width_helper(
        DiagnosticLocation const& loc
    ) noexcept -> std::size_t {
        std::size_t max_line_number{};

        if (loc.is_location_item()) {
            auto const& temp = loc.get_as_location_item();
            auto const line_number = static_cast<std::size_t>(temp.line_number);
            auto lines = std::count(temp.source.begin(), temp.source.end(), '\n');
            max_line_number = std::max( {
                line_number,
                line_number + static_cast<std::size_t>(std::max(lines, 0l)),
                max_line_number
            });
        } else {
            auto const& temp = loc.get_as_location_tokens();
            for (auto const& line: temp.lines) {
                auto const line_number = static_cast<std::size_t>(line.line_number);
                auto lines = std::accumulate(line.tokens.begin(), line.tokens.end(), 0zu, [](auto res, auto const& item) {
                    auto token = item.token.to_borrowed();
                    return res + static_cast<std::size_t>(std::max(std::count(token.begin(), token.end(), '\n'), 1l));
                });

                max_line_number = std::max( {
                    line_number,
                    line_number + lines,
                    max_line_number
                });
            }
        }

        return max_line_number;
    }

    static constexpr inline auto count_digits(std::size_t num) noexcept -> std::size_t {
        std::size_t c{};
        while (num) {
            num /= 10;
            ++c;
        }
        return c;
    }

    template <typename K>
    static constexpr inline auto get_max_line_number_width(
        BasicDiagnostic<K> const& diag
    ) noexcept -> std::size_t {
        std::size_t max_line_number{ get_max_line_number_width_helper(diag.location) };

        for (auto const& d : diag.sub_diagnostic) {
            max_line_number = std::max(max_line_number, get_max_line_number_width_helper(d.location));
        }

        return std::max(count_digits(max_line_number), 2zu) + 1;
    }

    static constexpr inline auto render_line_start(
        TerminalScreen& screen,
        CursorPosition& cursor,
        std::size_t width,
        std::optional<unsigned> line_number = {}
    ) -> void {
        if (line_number) {
            auto num = *line_number;
            auto const digits = count_digits(num);
            auto padding = width - digits;
            screen.pad_with(" ", cursor, static_cast<unsigned>(padding));
            auto str = std::to_string(num);
            screen.write(str, cursor, { .color = line_color });
            screen.write(" |", cursor, { .color = line_color });
        } else {
            screen.pad_with(" ", cursor, static_cast<unsigned>(width));
            screen.write(" :", cursor, { .color = line_color });
        }
    }

    static constexpr inline auto render_file_info(
        TerminalScreen& screen,
        CursorPosition& cursor,
        std::size_t width,
        DiagnosticLocation const& loc
    ) -> void {
        if (loc.filename.empty()) return;
        auto [line, col] = loc.get_line_info();
        if (line == 0 || col == 0) return;

        while (width--) screen.write(" ", cursor);
        screen.write("--> ", cursor, { .color = line_color });
        screen.write(loc.filename, cursor);
        screen.write(":", cursor);
        screen.write(std::to_string(line), cursor);
        screen.write(":", cursor);
        screen.write(std::to_string(col), cursor);
        cursor.next_line();
    }

         
    static constexpr inline auto sort_context_span(
        core::SmallVec<BasicDiagnosticMessage, 2>& context
    ) noexcept -> void {
        for (auto& item: context) {
            std::stable_sort(item.spans.begin(), item.spans.end(), [](auto const&l, auto const& r) {
               return l.start() < r.start();
            });
        }

        std::stable_sort(context.begin(), context.end(), [](auto const& l, auto const& r) {
            return l.spans.back().start() < r.spans.back().start();
        });
    }


    /**
     * @param skip_line_cutoff 0: render every line, or any number would be use for removing lines.
     */
    static constexpr inline auto render_diagnostic_context_text(
        TerminalScreen& screen,
        CursorPosition& cursor,
        std::size_t width,
        DiagnosticRenderContext const& context
    ) -> std::vector<DisplaySpan> {
        std::vector<DisplaySpan> markers;
        auto const& lines = context.lines;
        if (lines.empty()) return markers;

        auto next_line_number = lines[0].line_number;
        for (auto l = 0zu; l < lines.size(); ++l) {
            auto const& line = lines[l];
            auto const& infos = line.infos;

            if (next_line_number != line.line_number) {
                screen.write("...", cursor, { .color = line_color });
                auto padding = static_cast<unsigned>(width - 3zu - 1zu);
                screen.pad_with(" ", cursor, padding);
                cursor.next_line();
            }

            next_line_number = line.line_number + 1;

            render_line_start(screen, cursor, width, line.line_number);
            screen.pad_with(" ", cursor, 1); 
            auto const padding = cursor.col;

            for (auto i = 0zu; i < infos.size(); ++i) {
                auto info = infos[i];
                if (info.text.empty()) continue;
                if (info.is_skippable()) {
                    screen.write_escape(info.text.to_borrowed(), cursor, info.style);
                    continue;
                }
                
                auto old_c = cursor;
                auto n = screen.try_write_escape(info.text.to_borrowed(), cursor, info.style);
                if (n == info.text.size()) {
                    markers.emplace_back(old_c, l, i);
                    continue;
                }
                
                screen.pad_with(" ", old_c, static_cast<unsigned>(cursor.col - old_c.col));
                
                cursor.next_line();
                cursor.col = padding;
                markers.emplace_back(cursor, l, i);
                screen.write_escape(info.text.to_borrowed(), cursor, info.style);
            }
            cursor.next_line();
        }
        
        std::stable_sort(markers.begin(), markers.end(), [](auto l, auto r) {
            return l.cursor.col < r.cursor.col;
        });
        
        return markers;
    }

    struct MessagePosition {
        std::optional<CursorPosition> cursor;
        std::size_t marked_item_id{};
        std::size_t message_id{};
        std::size_t span_id{};
    };

    static inline auto place_messages(
        TerminalScreen& screen,
        std::size_t width,
        DiagnosticRenderContext const& context,
        std::vector<DisplaySpan> const& marked_items,
        core::SmallVec<BasicDiagnosticMessage, 2>& messages,
        BoundingBox box
    ) -> std::vector<MessagePosition> {
        std::vector<MessagePosition> res;
        auto const size = std::accumulate(marked_items.begin(), marked_items.end(), 0zu, [&context](auto res, DisplaySpan const& item) {
            return res + context.lines[item.context_index].infos.size();
        });

        res.reserve(size);
        auto const init_padding = box.min_x + 1;
        auto const max_cols = screen.cols() - init_padding;
        auto last_row = screen.get_rows_to_print();
        {
            auto cursor = CursorPosition { .row = last_row, .col = 0 };
            render_line_start(screen, cursor, width);
            ++last_row;
        }
        auto const half_col = max_cols >> 1; 
        std::unordered_map<std::ptrdiff_t, CursorPosition> cache;

        for (auto mit = marked_items.rbegin(); mit != marked_items.rend(); ++mit) {
            auto const& el = *mit;
            auto marked_id = static_cast<std::size_t>(marked_items.rend() - mit) - 1;
            auto const& line = context.lines[el.context_index];
            
            auto const& info = line.infos[el.item_index];
            auto parent_span = messages[info.ids[0].message_id].spans[info.ids[0].span_id];
            auto abs_pos = static_cast<std::ptrdiff_t>(parent_span.start());

            for (auto const& i_id: info.ids) {
                auto m_id = i_id.message_id;
                auto s_id = i_id.span_id;
                auto const& message = messages[m_id];
                auto text = message.message.to_borrowed();
                auto span = message.spans[s_id].shift(-abs_pos);

                auto text_size = text.size();
                auto cursor = el.cursor;
                cursor.col += span.start();

                // Requiremnet:
                //  1. Line width must be at least half the column width if it's lies in the second half of the screen.
                //  2. Line must not exceed 2 lines unless space is less. 

                // Case 1:
                //                    Center
                // |--------------------|-------------------------|
                // |                    |           x             |
                // |                    |           ^             |
                // |                    |           |             |
                // |                    |           yyyyyyyyyyyyyy|
                // |                    |           yyyyyyyyyyyyyy|
                // |                    |           yyyyyyyyyyyyyy|
                // |                    |                         |
                // |--------------------|-------------------------|
                // If the wrapping takes more than 2 lines shift the message back until it fits the
                // between 2 lines. 
                // Case 2:
                //                    Center
                // |--------------------|-------------------------|
                // |            x       |                         |
                // |            ^       |                         |
                // |            |       |                         |
                // |            |-yyyyyyyyyyyyyyyyyyyyy           |
                // |            |-yyyyyyyyyyyyyyyyyyyyy           |
                // |                    |                         |
                // |                    |                         |
                // |                    |                         |
                // |--------------------|-------------------------|
                // 
                
                auto address = reinterpret_cast<std::ptrdiff_t>(text.data());
                auto cache_it = cache.find(address);
                
                auto position = MessagePosition {
                    .cursor = cursor,
                    .marked_item_id = marked_id,
                    .message_id = m_id,
                    .span_id = s_id
                };


                if (text_size == 0) {
                    position.cursor = std::nullopt;
                    res.push_back(position);
                    continue;
                }
                
                // If the message is not drawn, then draw it
                if (cache_it == cache.end()) {
                    auto item_cursor = cursor;
                    // Try to fit the message within the bounds
                    if (text_size != 0) {
                        auto remaining_space = (max_cols - item_cursor.col);
                        auto new_c = item_cursor;
                        new_c.row = last_row;
                        // there is no remaining space so we assign the text at the middle of the column.
                        if (remaining_space == 0) {
                            auto half = text_size >> 1;
                            auto line_width = std::min(half_col, half);
                            // |.....................................................|
                            // [-------------------- half_col -----------------------]
                            // [--(half_col - line_width)--][------ new_c.col -------]
                            new_c.col = half_col + (half_col - line_width);                        
                        } else {
                            // Divide the text into chunks of remaining space, and check if
                            // it exceeding 2 to readjust the position. 
                            auto lines_required = text_size / remaining_space;
                            if (lines_required > 2) {
                                auto rem = std::min(text_size % remaining_space, half_col);
                                if (item_cursor.col > half_col) {
                                    auto start = std::max(item_cursor.col - rem, half_col);
                                    new_c.col = start;
                                } else {
                                    new_c.col = item_cursor.col;
                                } 
                            } else {
                                new_c.col = item_cursor.col;
                            }
                            new_c.col = std::max(new_c.col, box.min_x + 4);
                        }

                        
                        // 1. If the text cannot be inserted at the giving position with 4 cell padding, we move to
                        //    the next line pr append a new row. 
                        // 2. If we can shift the text 2 cell back, we shift it so that we can have clear text separation. 
                        while (!screen.can_insert_between_cols(new_c.row, new_c.col, new_c.col + text_size + 4)) {
                            screen.try_fix_row(new_c.row);
                            auto& cell = screen(new_c);
                            auto should_shift = (!cell.empty_or_space());
                            if (get_z_index(message) == cell.style.z) should_shift = false;
                            new_c.row += 1;
                            if (should_shift) new_c.col = std::max(init_padding + 4, new_c.col - 2);
                            if (screen.rows() <= new_c.row) screen.append_rows();
                        }
                        
                        position.cursor = new_c;
                        res.push_back(position);
                        auto temp_c = new_c;
                        temp_c.col = 0;
                    
                        auto const padding = static_cast<unsigned>(new_c.col);
                        screen.pad_with(" ", temp_c, padding);
                        temp_c.col = 0;
                        render_line_start(screen, temp_c, width);
                        
                        auto style = Style { .color = get_color(message), .z = get_z_index(message, 10), .end_padding = 4, .prevent_wordbreak = true };
                        screen.write("-", new_c, style);
                        screen.write(text, new_c, style, [&screen, width, padding](auto& pos, auto&) {
                            screen.pad_with(" ", pos, padding + 2);
                            pos.col = 0;
                            render_line_start(screen, pos, width);
                            pos.col = padding + 2;
                        });
                        cache[address] = *position.cursor;
                    }
                } else {
                    position.cursor = cache_it->second;
                    res.push_back(position);  
                }
                              
            }            
        }        

        return res;
    }

    
    struct Marker {
        CursorPosition marker_pos;
        MessagePosition message_positions;
        MarkerKind kind;
        MarkerArrowDirection dir{};
        bool is_main_marker{false};
    };

   
    static inline auto generate_markers_for_render(
        core::SmallVec<BasicDiagnosticMessage, 2> const& messages,
        std::vector<DisplaySpan> const& marked_items,
        std::vector<MessagePosition> const& message_position
    ) -> std::vector<Marker> {
        std::vector<Marker> res;
        res.reserve(message_position.size());


        for (auto i = 0zu; i < marked_items.size(); ++i) {
            auto const& item = marked_items[i];
            
            auto cursor = item.cursor;
            
            for (auto const& pos: message_position) {
                if (pos.marked_item_id != i) continue;
                BasicDiagnosticMessage const& m = messages[pos.message_id];
                res.push_back({ .marker_pos = cursor, .message_positions = pos, .kind = to_marker_kind(m), .is_main_marker = m.is_marker });
            }
        }
        
        return res;
    }

    static inline auto insert_space_between_rows(
        TerminalScreen& screen,
        std::size_t width,
        std::vector<Marker>& markers,
        std::size_t row,
        std::size_t count
    ) -> void {
        for (auto& m: markers) {
            auto c = m.message_positions.cursor;
            if (c) {
                c->row += count;
                m.message_positions.cursor = c;
            }
            
            if (m.marker_pos.row <= row) continue;
            m.marker_pos.row += count;
        }

        while (count--) {
            screen.insert_row(row + 1);
            auto c = CursorPosition(row + 1, 0);
            render_line_start(screen, c, width);
        }
    }
    
    static inline auto ensure_marker_spaces(
        TerminalScreen& screen,
        std::size_t width,
        std::vector<Marker>& markers
    ) -> void {
        
        using marker_t = std::array<bool, static_cast<std::size_t>(MarkerKind::Size)>;

        { // Adjusting spaces and assigning position for the markers

            { // Allocate spaces
                std::unordered_map<CursorPosition, marker_t, CursorPositionHash> markers_required;

                for(auto const& m: markers) {
                    auto index = static_cast<std::size_t>(m.kind);
                    auto& mask = markers_required[m.marker_pos];
                    mask[index] = true;
                }
                
                std::map<std::size_t, std::size_t> spaces;

                for (auto const& [k, mask]: markers_required) {
                    auto count = std::accumulate(mask.begin(), mask.end(), 0zu, [](auto res, auto f){
                        return res + static_cast<std::size_t>(f);
                    });
                    auto old_count = spaces[k.row];
                    spaces[k.row] = std::max(old_count, count);
                }

                std::vector<std::pair<std::size_t, std::size_t>> temp_spaces(spaces.begin(), spaces.end());
                
                for (auto i = 0zu; i < temp_spaces.size(); ++i) {
                    auto [r, count] = temp_spaces[i];
                    
                    // NOTE: These values are always in ascending order.
                    for (auto j = i + 1; j < temp_spaces.size(); ++j) {
                        auto& [k, _] = temp_spaces[j];
                        k += count;
                    }

                    insert_space_between_rows(screen, width, markers, r, count);
                }
            }
            
            std::stable_sort(markers.begin(), markers.end(), [](Marker l, Marker r) {
                auto lr = l.marker_pos.row;
                auto rr = r.marker_pos.row;
                if (lr == rr) return l.kind > r.kind;
                return lr > rr; 
            });

            auto markers_clone = markers;
            
            std::unordered_map<CursorPosition, std::pair<marker_t, CursorPosition>, CursorPositionHash> map;

            for (auto i = 0zu; i < markers_clone.size(); ++i) {
                auto const& marker = markers_clone[i];
                if (auto it = map.find(marker.marker_pos); it != map.end()) {
                    auto [mask, cursor] = it->second;
                    auto index = static_cast<std::size_t>(marker.kind);
                    if (!mask[index]) {
                        mask[index] = true;
                        ++cursor.row;
                        it->second = { mask, cursor };
                    }
                    markers[i].marker_pos = cursor;
                } else {
                    auto cursor = marker.marker_pos;
                    ++cursor.row;
                    marker_t mask {false};
                    mask[static_cast<std::size_t>(marker.kind)] = true;
                    map[marker.marker_pos] = {mask, cursor};
                    markers[i].marker_pos = cursor;
                } 
            }

            std::stable_sort(markers.begin(), markers.end(), [](Marker l, Marker r) {
                return l.marker_pos.to_pair() < r.marker_pos.to_pair(); 
            });

            for (auto i = 0zu; i < markers.size(); ++i) {
                for (auto j = i + 1; j < markers.size(); ++j) {
                    auto l = markers[i].message_positions;
                    auto r = markers[j].message_positions;
                    if ((l.message_id == r.message_id) && (l.span_id == r.span_id)) {
                        markers[i].marker_pos.row = markers[j].marker_pos.row;
                    }
                }
            }            
        }
    }

    static inline auto ensure_arrow_spaces(
        TerminalScreen& screen,
        std::size_t width,
        DiagnosticRenderContext const& context,
        std::vector<DisplaySpan> const& marked_items,
        std::vector<Marker>& markers,
        BoundingBox box
    ) -> void {
        using marker_t = std::array<bool, static_cast<std::size_t>(MarkerKind::Size)>;
        // - Ensure space for start arrows
        // - "<-", "->", or
        // - "^"
        //   "|"
        
        // Marker item index -> number of markers
        std::unordered_map<std::size_t, marker_t> markers_count;
        markers_count.reserve(marked_items.size());

        auto const start_width = box.min_x + 1;

        for (auto const& m: markers) {
            auto mask = markers_count[m.message_positions.marked_item_id];
            auto index = static_cast<std::size_t>(m.kind);
            mask[index] = true;
            markers_count[m.message_positions.marked_item_id] = mask;
        }

        for (auto [k, v]: markers_count) {
            auto count = std::accumulate(v.begin(), v.end(), 0zu, [](auto res, auto f){
                return res + static_cast<std::size_t>(f);
            });
            if (count == 0) continue;

            auto const& item = marked_items[k];
            auto text = context.lines[item.context_index].infos[item.item_index].text;
            for (auto& m: markers) {
                if (m.message_positions.marked_item_id != k) continue;
                if (!m.message_positions.cursor) continue;
                auto start_cursor = m.marker_pos;
                auto end_cursor = start_cursor;
                end_cursor.col += text.size();
            
                static constexpr auto vertical_arrow_len = 2zu;
                static constexpr auto horizontal_arrow_len = 1zu;
            

                // Case 1: if the number of markers is 1, we try to fit left arrow then right arrow then bottom arrow. 
                if (count == 1) {
                    auto left_cursor = start_cursor;
                    left_cursor.row = std::max(1zu, left_cursor.row) - 1;
                    // NOTE: This should never be less than 2.
                    left_cursor.col -= vertical_arrow_len;
                    
                    
                    if (start_width < left_cursor.col && screen.can_insert_between_cols(left_cursor.row, left_cursor.col, start_cursor.col)) {
                        m.dir = MarkerArrowDirection::Left;
                        m.marker_pos = left_cursor; // Set the cursor the right position
                        continue;
                    }


                    auto right_cursor = end_cursor;
                    right_cursor.row = std::max(1zu, right_cursor.row) - 1;
                    right_cursor.col += vertical_arrow_len;

                    if (right_cursor.col < screen.cols()) {
                        if (screen.can_insert_between_cols(right_cursor.row, end_cursor.col, right_cursor.col)) {
                            m.dir = MarkerArrowDirection::Right;
                            m.marker_pos = right_cursor;
                            continue;
                        }
                    }
                }
                
                m.dir = MarkerArrowDirection::Down;

                // Case 2: if the number of markers is more than 2 or no vertical space, we always chose bottom arrow. 
                while(start_cursor.row + horizontal_arrow_len >= screen.rows()) screen.append_rows();
                for (auto i = 0zu; i < horizontal_arrow_len; ++i) {
                    auto const row = start_cursor.row + i + 1;
                    auto spaces = horizontal_arrow_len - i;
                    if (!screen.can_insert_between_cols(row, start_cursor.col, start_cursor.col + 1)) {
                        insert_space_between_rows(screen, width, markers, row - 1, spaces);
                        break;
                    }
                }
            }
        }
    }

    static inline auto render_markers(
        TerminalScreen& screen,
        std::size_t width,
        DiagnosticRenderContext const& context,
        std::vector<DisplaySpan> const& marked_items,
        std::vector<Marker> const& markers
    ) -> void {
        for (auto const& marker: markers) {
            auto const& m_item = marked_items[marker.message_positions.marked_item_id];
            auto const& info = context.lines[m_item.context_index].infos[m_item.item_index];
            auto style = info.style;
            style.is_strike = false;
            style.is_dim = false;
            style.is_bold = false;
            style.is_bg = false;
            style.color = get_color(marker.kind);
            style.z = get_z_index(marker.kind);
            auto size = static_cast<unsigned>(info.text.size());
            auto m = get_marker_char(marker.kind);
            if (marker.is_main_marker) {
                m = '^';
            }
            auto cursor = marker.marker_pos;
            cursor.col = 0;
            render_line_start(screen, cursor, width);
            
            auto padding = static_cast<unsigned>(marker.marker_pos.col - cursor.col);
            screen.pad_with(" ", cursor, padding);
            cursor = marker.marker_pos;
            screen.pad_with(std::string_view(&m, 1), cursor, size, style); 
        }
    }

    static inline auto render_path_between_marker_and_message(
        TerminalScreen& screen,
        std::vector<Marker> const& markers,
        BoundingBox box
    ) -> void {
        enum class Direction: char {
            None = ' ',
            Vertical = '|',
            Horizontal = '-',
            DiagonalLeftRight = '/',
            DiagonalRightLeft = '\\',
            Intersection = '+',
            DiagonalIntersection = 'x'
        };

        std::vector<std::tuple<CursorPosition, std::vector<std::size_t>, MarkerArrowDirection>> sorted_messages;
        
        {
            // Message position -> Vector<Index into Markers>
            std::unordered_map<CursorPosition, std::vector<std::size_t>, CursorPositionHash> messages_cache;

            // TODO: Markers can be split into two or more markers that renders multiple arrows.
            //       To avoid this issue, we cache the processed message then do not render them again. 
            std::unordered_set<CursorPosition, CursorPositionHash> processed;
            processed.reserve(sorted_messages.size());


            for (auto i = 0zu; i < markers.size(); ++i) {
                auto const& marker = markers[i];
                auto cursor = marker.message_positions.cursor;
                if (!cursor) continue;
                auto pos = CursorPosition(marker.message_positions.message_id, marker.message_positions.span_id);
                if (processed.count(pos) != 0) continue;
                processed.insert(pos);
            
                auto& temp = messages_cache[*cursor];
                temp.push_back(i);
            }

            sorted_messages.reserve(messages_cache.size());
            
            for (auto& [k, v]: messages_cache) {
                if (v.empty()) continue;
                std::array<std::size_t, static_cast<std::size_t>(MarkerArrowDirection::Size)> mask{0};
                for (auto index: v) {
                    auto dir = markers[index].dir;
                    mask[static_cast<std::size_t>(dir)] += 1;
                }
                auto max_it = std::max_element(mask.begin(), mask.end());
                auto arrow = static_cast<MarkerArrowDirection>(std::max(std::distance(mask.begin(), max_it), 0l));
                if (arrow != MarkerArrowDirection::Left && arrow != MarkerArrowDirection::Right) {
                    arrow = MarkerArrowDirection::Left;
                }
                sorted_messages.emplace_back(k, std::move(v), arrow);
            }

            std::stable_sort(sorted_messages.begin(), sorted_messages.end(), [](auto const& l, auto const& r){
                return std::get<0>(l).col > std::get<0>(r).col;
            });
        }
       
        // Render arrows
        for (auto& [k, v, _]: sorted_messages) {
            for (auto index: v) {
                auto marker = markers[index];
                if (!marker.message_positions.cursor) continue;

                auto style = Style {  .color = get_color(marker.kind), .z = get_z_index(marker.kind) };
                switch (marker.dir) {
                    case MarkerArrowDirection::Down: {
                        auto c = marker.marker_pos;
                        switch (marker.kind) {
                            case MarkerKind::Remove: case MarkerKind::Insert: break;
                            default: {
                                screen.write("^", c, style);
                                c = marker.marker_pos;
                            }
                        }
                        ++c.row;
                        screen.write("|", c, style);
                    } break;
                    case MarkerArrowDirection::Left: {
                        auto c = marker.marker_pos;
                        screen.write("->", c, style);
                    } break;
                    case MarkerArrowDirection::Right: {
                        auto c = marker.marker_pos;
                        c.col -= 2;
                        screen.write("<-", c, style);
                    } break;
                    default: continue;
                }
            }
            std::stable_sort(v.begin(), v.end(), [&markers](std::size_t l_idx, std::size_t r_idx) {
                auto l = markers[l_idx].marker_pos;
                auto r = markers[r_idx].marker_pos;
                return l.row < r.row;
            });
            
        }
   

        auto const find_path = [&screen](
            CursorPosition start,
            CursorPosition dest,
            MarkerArrowDirection bias,
            Style style,
            BoundingBox box
        ) -> std::vector<CursorPosition> {

            // This function allows to cut off invalid search space branches.
            // 
            // 1. If cell is empty, then weight is 0.
            // 2. If cell has some char, then weight is always some high number.
            // 3. If there is an intersection, then weight is 1.
            // 4. If we find the cell with the same id, we return -10 to incentivize to use this cell.
            auto const intersection_weight = [&screen, style](CursorPosition pos) -> int {
                auto& cell = screen(pos);
                if  (cell.style.id == style.id) return -10;
                if (cell.empty() || cell.to_view() == " ") return 0;
                
                if (cell.len != 1) return 100;
                
                auto c = static_cast<Direction>(cell.c[0]);
                switch (c) {
                    case Direction::Vertical: 
                    case Direction::Horizontal:
                    case Direction::DiagonalLeftRight:
                    case Direction::DiagonalRightLeft:
                    case Direction::Intersection:
                    case Direction::DiagonalIntersection: return 1;
                    default: return 100;
                }
                
            };
            auto const pad_with = [&screen, &intersection_weight, &style](Direction dir, CursorPosition pos, std::size_t padding) {
                for (auto i = 0zu; i < padding; ++i) {
                    auto const& cell = screen(pos);
                    auto weight = intersection_weight(pos);
                    bool should_show_intersection = (weight == 1) && (cell.style.z != style.z) && (cell.c[0] != static_cast<char>(dir));
                    
                    if (style.id == cell.style.id) should_show_intersection = false;

                    if (should_show_intersection) {
                        auto c = static_cast<char>(Direction::Intersection);
                        auto text = std::string_view(&c, 1);
                        screen.write(text, pos, Style { .z = 100 });
                    } else {
                        if (weight == 100) continue;
                        auto c = static_cast<char>(dir);
                        auto text = std::string_view(&c, 1);
                        screen.write(text, pos, style);
                    }
                }
            };

            auto const is_valid_position = [&intersection_weight](CursorPosition c) {
                return intersection_weight(c) < 2;
            };

            auto const can_reach_dest = [dest, &is_valid_position, box](CursorPosition c, MarkerArrowDirection dir, auto&& fn) -> bool {
                switch (dir) {
                    // Check all the vertical cells to test if there is any invalid cell. 
                    case MarkerArrowDirection::Up: {
                        if (box.min_y > c.row) return false;
                        if (dest.row > c.row) return false;
                        for (auto i = dest.row; i <= c.row; ++i) {
                            auto cr = CursorPosition(i, c.col);
                            if (cr != dest) { 
                                fn(cr);
                            }
                            if (!is_valid_position(cr)) return false;
                        }
                        return true;
                    } break;
                    // Check all the vertical cells to test if there is any invalid cell. 
                    case MarkerArrowDirection::Down: {
                        if (box.max_y <= c.row) return false;
                        if (c.row < dest.row) return false;
                        for (auto i = c.row; i <= dest.row; ++i) {
                            auto cr = CursorPosition(i, c.col);
                            if (cr != dest) { 
                                fn(cr);
                            }
                            if (!is_valid_position(cr)) return false;
                        }
                        return true;
                    } break;
                    // Check all the horizontal  cells to test if there is any invalid cell. 
                    case MarkerArrowDirection::Left: {
                        if (box.min_x > c.col) return false;
                        if (c.col < dest.col) return false;
                        for (auto i = dest.col; i <= c.col; ++i) {
                            auto cr = CursorPosition(c.row, i);
                            if (cr != dest) { 
                                fn(cr);
                            }
                            if (!is_valid_position(cr)) return false;
                        }
                        return true;
                    } break;
                    // Check all the horizontal  cells to test if there is any invalid cell. 
                    case MarkerArrowDirection::Right: {
                        if (box.max_x <= c.col) return false;
                        if (c.col > dest.col) return false;
                        for (auto i = c.col; i <= dest.col; ++i) {
                            auto cr = CursorPosition(c.row, i);
                            if (cr != dest) { 
                                fn(cr);
                            }
                            if (!is_valid_position(cr)) return false;
                        }
                        return true;

                    } break;
                    case MarkerArrowDirection::None: return true;
                    default: break;    
                }
                return false;
            };

            if (start.col == dest.col) {
                if (screen.can_insert_between_cols(start.row - 1, start.col, start.col + 1)) {
                    start.row = std::max(box.min_y, start.row - 1);
                }
            }
           

            auto const draw_line = [&](CursorPosition start, CursorPosition end) {
                if (start.col == end.col) {
                    auto r1 = std::min(start.row, end.row);
                    auto r2 = std::max(start.row, end.row);
                    for (; r1 <= r2; ++r1) pad_with(Direction::Vertical, { r1, start.col }, 1);
                } else {
                    auto c1 = std::min(start.col, end.col);
                    auto c2 = std::max(start.col, end.col);
                    pad_with(Direction::Horizontal, {start.row, c1}, c2 - c1);
                }
            };

            std::vector<CursorPosition> min_path, current_path;
            int min_intersection{std::numeric_limits<int>::max()}, current_intersection{};

            auto const copy_path = [](auto& out, auto const& in) {
                out.resize(in.size());
                std::copy(in.begin(), in.end(), out.begin());
            };

            auto const get_next_dir = [dest](CursorPosition start, MarkerArrowDirection dir) -> MarkerArrowDirection {
                switch (dir) {
                    case MarkerArrowDirection::Right: 
                    case MarkerArrowDirection::Left:  {
                        if (start.row > dest.row) return MarkerArrowDirection::Up;
                        else if (start.row < dest.row) return MarkerArrowDirection::Down;
                    } break;
                    case MarkerArrowDirection::Up:
                    case MarkerArrowDirection::Down: {
                        if (start.col > dest.col) return MarkerArrowDirection::Left;
                        else if (start.col < dest.col) return MarkerArrowDirection::Right;  
                    } break;
                    default: break;
                }
                return MarkerArrowDirection::None;
            };

            auto const cols = box.max_x - box.min_x;
            auto const rows = box.max_y - box.min_y;

            // Vec<(Vertical, Horizontal)>
            auto visited = std::vector<std::pair<bool, bool>>(rows * cols, { false, false });

            auto const at_visited = [&visited, box, &cols](CursorPosition pos, MarkerArrowDirection dir) -> bool& {
                auto row = pos.row - box.min_y;
                auto col = pos.col - box.min_x;
                auto& res = visited[row * cols + col];
                switch (dir) {
                    case MarkerArrowDirection::Right:
                    case MarkerArrowDirection::Left: return res.second;
                    default: return res.first;
                }
            };


            auto start_dir = bias;

            auto const find_path_helper = [&](
                auto&& self,
                CursorPosition start,
                MarkerArrowDirection bias
            ) -> void {
                
                if (start == dest) {
                    if (min_path.empty()) {
                        copy_path(min_path, current_path);
                    } else {
                        if (min_intersection <= current_intersection) return;
                        if (current_path.size() >= min_path.size()) return;
                        copy_path(min_path, current_path);
                    }

                    min_intersection = current_intersection;
                    return;
                }
                if (bias == MarkerArrowDirection::None || (min_intersection <= current_intersection)) return; 

                auto next_dir = get_next_dir(start, bias);

                auto const& can_reach_helper = [&can_reach_dest, &intersection_weight, next_dir](CursorPosition start) {
                    auto intersecionts = 0z;
                    auto count = [&intersecionts, &intersection_weight](CursorPosition pos) { 
                        intersecionts += intersection_weight(pos);
                    };

                    return std::make_pair(can_reach_dest(start, next_dir, count), intersecionts);
                };
            
                auto const iter = [&self, dest, &copy_path, start_dir, &can_reach_helper, box, next_dir, &at_visited, &min_intersection, &current_path, &current_intersection](CursorPosition start, MarkerArrowDirection dir) {
                    std::vector<std::tuple<CursorPosition, int, std::size_t>> options; 
                    auto info = can_reach_helper(start);

                    auto const get_distance = [dir, dest](CursorPosition start) {
                        switch (dir) {
                            case MarkerArrowDirection::Up: 
                            case MarkerArrowDirection::Down: return std::max(dest.row, start.row) - std::min(dest.row, start.col);
                            case MarkerArrowDirection::Left: 
                            case MarkerArrowDirection::Right: return std::max(dest.col, start.col) - std::min(dest.col, start.col);                         
                            default: return std::numeric_limits<std::size_t>::max();
                        }
                    };

                    auto const is_valid_direction = [start_dir, dest](CursorPosition pos) {
                        switch (start_dir) {
                            case MarkerArrowDirection::Up: return dest.row <= pos.row;
                            case MarkerArrowDirection::Down: return dest.row >= pos.row;
                            case MarkerArrowDirection::Left: return dest.col >= pos.col;
                            case MarkerArrowDirection::Right: return dest.col <= pos.col;
                            default: return true;
                        };
                    };
                   
                    auto temp_start = start;
                    auto temp = start;
                    // 1. Try to find all the possible options for search space to reduce search space.
                    while (temp != temp_start.go(dir, box)) {
                        auto [can_reach, intersec] = info;
                        if (
                                !at_visited(temp_start, dir) && 
                                is_valid_direction(temp_start) && 
                                can_reach && 
                                intersec < 100 &&
                                (intersec < min_intersection)
                        ) {
                            auto dist = get_distance(temp);
                            options.push_back({temp_start, intersec, dist});
                        }

                        if (temp == dest) {
                            break;
                        }

                        temp = temp_start.go(dir, box);
                        temp_start = temp;
                        info = can_reach_helper(temp_start);
                    }

                    // 2. Sort for the best candidates (Greedy Method)
                    std::sort(options.begin(), options.end(), [](auto l, auto r){
                        auto [lv, li, ld] = l;
                        auto [rv, ri, rd] = r;
                        (void) lv;
                        (void) rv;
                        if (li == ri) return ld < rd; 
                        return li < ri;
                    });
                    
                    // store the previous current path in case if we use the previously rendered path.
                    core::SmallVec<CursorPosition, 5> old_path;

                    // 3. Visit all the options
                    for (auto [pos, intersec, _d]: options) {
                        at_visited(pos, dir) = true;
                        auto old_inter = current_intersection;
                        current_intersection += intersec;

                        // If we intersect with the previously rendered path, we discard all the
                        // history and start a new from the current cell. 
                        if (intersec < 0) {
                            current_intersection = 0;
                            copy_path(old_path, current_path);
                            current_path.clear();
                        }
                        current_path.push_back(pos);

                        self(self, pos, next_dir);

                        // Restore the path
                        if (intersec < 0) {
                            copy_path(current_path, old_path);
                        }

                        current_intersection = old_inter;
                    }
                    
                    // 3.5. There is no options then try visit vertically.
                    if (options.empty()) {
                        auto& res = at_visited(start, MarkerArrowDirection::Up);
                        if (res) return;
                        res = true;
                        self(self, start, MarkerArrowDirection::Up);
                    }
                };
                
                iter(start, bias);
            };


            current_path.push_back(start);

            find_path_helper(find_path_helper, start, bias);

            if (!min_path.empty()) {
                for (auto i = 0zu; i < min_path.size() - 1; ++i) {
                    draw_line(min_path[i], min_path[i + 1]);
                }
            }

            return min_path;
        };

    
        for (auto const& [message_pos, targets, bias]: sorted_messages) {
            auto back = markers[targets.back()];
            auto start = message_pos;
            auto dest = back.marker_pos;
            if (back.dir == MarkerArrowDirection::Down) {
                dest.row = std::min(box.max_y, dest.row + 1);
            }
        
            auto style = Style { .color = get_color(back.kind), .z = get_z_index(back.kind), .id = back.message_positions.message_id };
            auto path = find_path(start, dest, bias, style, box);
            
            for (auto it = targets.begin(); it != targets.end() - 1; ++it) {
                auto el = markers[*it];
                dest = el.marker_pos;
                if (el.dir == MarkerArrowDirection::Down) {
                    dest.row = std::min(box.max_y, dest.row + 1);
                }
                find_path(start, dest, bias, style, box);
            }
        }

    }

    static inline auto render_orphan_messages(
        TerminalScreen& screen,
        std::size_t width,
        CursorPosition& cursor,
        core::SmallVec<BasicDiagnosticMessage, 2>& messages
    ) -> void {
        cursor.row = screen.get_rows_to_print();
        cursor.col = 0;
    
        auto const has_orphans = std::count_if(messages.begin(), messages.end(), [](auto const& m) {
            return (m.spans.empty() && !m.message.empty());
        }) > 0;


        if (!has_orphans) {
            return;
        }
        
        render_line_start(screen, cursor, width);
        cursor.next_line();
        screen.pad_with(" ", cursor, static_cast<unsigned>(width) + 1);

        std::stable_sort(messages.begin(), messages.end(), [](BasicDiagnosticMessage const& l, BasicDiagnosticMessage const& r) {
            return get_z_index(l) < get_z_index(r);
        });

        for (auto const& m: messages) {
            if (!m.spans.empty()) continue;
            if (m.message.empty()) continue;
            auto name = diagnostic_level_to_string(m.level);
            auto style = Style { .color = get_color(m), .is_bold = true, .z = get_z_index(m) };
            screen.write("= ", cursor, style);
            screen.write(name, cursor, style);
            screen.write(": ", cursor, style);
            screen.write(m.message.to_borrowed(), cursor, { .keep_ident = true, .prevent_wordbreak = true });
        }
        cursor.next_line();
    }
 
    static inline auto render_location_helper(
        TerminalScreen& screen,
        CursorPosition& cursor,
        std::size_t width,
        DiagnosticRenderContext& context,
        core::SmallVec<BasicDiagnosticMessage, 2>& messages,
        BoundingBox box
    ) -> void {
        if (!context.lines.empty()) {
            auto marked_items = render_diagnostic_context_text(screen, cursor, width, context);
            auto message_position = place_messages(screen, width, context, marked_items, messages, box); 
            auto markers = generate_markers_for_render(messages, marked_items, message_position);
            ensure_marker_spaces(screen, width, markers);
            render_markers(screen, width, context, marked_items, markers);
            ensure_arrow_spaces(screen, width, context, marked_items, markers, box);
            box.max_x = screen.cols();
            box.max_y = screen.get_rows_to_print();
            render_path_between_marker_and_message(screen, markers, box);
        }
        render_orphan_messages(screen, width, cursor, messages);
    }

    static inline auto render_location_item(
        TerminalScreen& screen,
        CursorPosition& cursor,
        std::size_t width,
        DiagnosticLevel level,
        BasicDiagnosticLocationItem& loc,
        core::SmallVec<BasicDiagnosticMessage, 2>& messages,
        BoundingBox box
    ) -> void {
        if (!loc.span().empty()) { 
            messages.push_back(BasicDiagnosticMessage {
                .level = level,
                .spans = { loc.span() },
                .is_marker = true
            });
        }
        sort_context_span(messages);
        auto context = DiagnosticRenderContext::from_loaction_item(loc, messages);
        render_location_helper(
            screen,
            cursor,
            width,
            context,
            messages,
            box
        );
    }

    static inline auto render_location_tokens(
        TerminalScreen& screen,
        CursorPosition& cursor,
        std::size_t width,
        DiagnosticLevel level,
        DiagnosticLocationTokens& loc,
        core::SmallVec<BasicDiagnosticMessage, 2>& messages,
        BoundingBox box
    ) -> void {
        if (!loc.marker.empty()) { 
            messages.push_back(BasicDiagnosticMessage {
                .level = level,
                .spans = { loc.marker },
                .is_marker = true
            });
        }
        sort_context_span(messages);

        std::stable_sort(loc.lines.begin(), loc.lines.end(), [](auto const& l, auto const& r){
            return l.line_number < r.line_number;
        }); 
        
        auto context = DiagnosticRenderContext::from_tokens(loc, messages); 
        render_location_helper(
            screen,
            cursor,
            width,
            context,
            messages,
            box
        );
    }

    static constexpr inline auto render_source(
        TerminalScreen& screen,
        CursorPosition& cursor,
        std::size_t width,
        DiagnosticLevel level,
        DiagnosticLocation& loc,
        core::SmallVec<BasicDiagnosticMessage, 2>& context,
        BoundingBox box
    ) noexcept -> void {
        if (loc.is_location_item()) {
            render_location_item(screen, cursor, width, level, loc.get_as_location_item(), context, box);
        } else {
            render_location_tokens(screen, cursor, width, level, loc.get_as_location_tokens(), context, box);
        }
    }

    template<typename K>
    static auto render_subdiagnostics(
        TerminalScreen& screen,
        CursorPosition cursor,
        std::size_t width,
        core::SmallVec<SubDiagnosticMessage<K>, 0>& diag
    ) -> void {
        if (diag.empty()) return;

        for (auto& temp_diag: diag)  {
            render_error(
                screen,
                cursor,
                temp_diag.level,
                std::move(temp_diag.formatter)
            );
            
            render_file_info(
                screen,
                cursor,
                width,
                temp_diag.location
            );

            BoundingBox box {
                .min_x = width + 2,
                .min_y = screen.get_rows_to_print(),
                .max_x = screen.cols()
            };

            render_source(
                screen,
                cursor,
                width,
                temp_diag.level,
                temp_diag.location,
                temp_diag.context,
                box
            );
        }
    }

    /**
        - BasicDiagnostic
            - kind
            - level
            - formatter
            - location
                - filename
                - source
                    - BasicDiagnosticLocationItem
                        - source
                        - line_number
                        - column_number
                        - length
                    - DiagnosticLocationTokens
                        - lines[]
                            - tokens[]
                                - token
                                - color
                                - column_number
                                - is_bold
                            - line_number
            - context[]
                - text
                - level
                - spans[]
                - op
            - sub_diagnostic[]
                - kind
                - level
                - formatter
                - location
                    ...
                - context[]
                    ...
    */
    template <typename K>
    auto draw_diagnostics(BasicDiagnostic<K>&& diag) -> TerminalScreen {
        auto screen = TerminalScreen{};
        CursorPosition cursor{};

        render_error(screen, cursor, diag.level, std::move(diag.formatter));

        auto const max_line_number_width = get_max_line_number_width(diag);
        render_file_info(
            screen,
            cursor,
            max_line_number_width,
            diag.location
        );

        render_source(
            screen,
            cursor,
            max_line_number_width,
            diag.level,
            diag.location,
            diag.context,
            BoundingBox{
                .min_x = max_line_number_width + 2,
                .min_y = screen.get_rows_to_print(),
                .max_x = screen.cols()
            }
        );

        render_subdiagnostics(
            screen,
            cursor,
            max_line_number_width,
            diag.sub_diagnostic
        ); 
        
        return screen;
    };

} // namespace dark::internal

#endif // DARK_DIAGNOSTICS_PRINTER_HPP
