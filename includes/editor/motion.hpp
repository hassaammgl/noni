#pragma once

#include <editor/selection.hpp>
#include <editor/window.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

struct MotionResult
{
    Cursor dest{.line = 0, .column = 0};
    TextRange range{}; // from origin toward dest (exclusive end), for operators
    bool linewise = false;
    bool ok = true;
};

namespace Motion
{
    inline bool is_word_char(unsigned char c)
    {
        return std::isalnum(c) || c == '_';
    }

    inline MotionResult make_char_motion(Cursor from, Cursor to, const std::vector<std::string> &lines)
    {
        MotionResult r;
        r.dest = clamp_cursor_to_lines(to, lines);
        r.linewise = false;
        r.range.kind = SelectionKind::Character;

        Cursor a = clamp_cursor_to_lines(from, lines);
        Cursor b = r.dest;
        if (cursor_before(b, a) || cursor_equal(a, b))
        {
            r.range.start = b;
            r.range.end = a;
        }
        else
        {
            r.range.start = a;
            r.range.end = b;
        }
        return r;
    }

    inline MotionResult make_line_motion(Cursor from, Cursor to, int line_count)
    {
        MotionResult r;
        r.linewise = true;
        r.dest = to;
        if (line_count <= 0)
        {
            r.dest = {.line = 0, .column = 0};
            return r;
        }
        r.dest.line = std::clamp(to.line, 0, line_count - 1);
        r.dest.column = 0;
        r.range = normalize_line_range(from, r.dest, line_count);
        // Operator line motions use full lines between from and dest inclusive.
        return r;
    }

    inline bool is_word_cp(char32_t cp)
    {
        if (cp < 128)
            return std::isalnum(static_cast<unsigned char>(cp)) || cp == '_';
        // Non-ASCII: treat as a word character if not Zs/control.
        return cp > 32 && cp != 0xA0;
    }

    inline bool is_space_cp(char32_t cp)
    {
        return cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == 0xA0;
    }

    inline void set_preferred_display(Window &win, const std::string &line, int byte_col)
    {
        win.preferred_column() = TextMetrics::byte_to_display(line, static_cast<std::size_t>(byte_col));
    }

    inline MotionResult left(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c = win.cursor();
        if (lines.empty())
            return make_char_motion(c, c, lines);
        const auto &row = lines[static_cast<std::size_t>(c.line)];
        c.column = static_cast<int>(TextMetrics::prev_cp(row, static_cast<std::size_t>(c.column)));
        set_preferred_display(win, row, c.column);
        return make_char_motion(win.cursor(), c, lines);
    }

    inline MotionResult right(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c = win.cursor();
        if (lines.empty())
            return make_char_motion(c, c, lines);
        const auto &row = lines[static_cast<std::size_t>(c.line)];
        if (static_cast<std::size_t>(c.column) < row.size())
            c.column = static_cast<int>(TextMetrics::next_cp(row, static_cast<std::size_t>(c.column)));
        set_preferred_display(win, row, c.column);
        return make_char_motion(win.cursor(), c, lines);
    }

    inline MotionResult up(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c = win.cursor();
        if (c.line > 0)
            c.line--;
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(c.line)];
            c.column = static_cast<int>(
                TextMetrics::display_to_byte(row, win.preferred_column()));
        }
        c = clamp_cursor_to_lines(c, lines);
        return make_line_motion(win.cursor(), c, static_cast<int>(lines.size()));
    }

    inline MotionResult down(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c = win.cursor();
        if (!lines.empty() && c.line + 1 < static_cast<int>(lines.size()))
            c.line++;
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(c.line)];
            c.column = static_cast<int>(
                TextMetrics::display_to_byte(row, win.preferred_column()));
        }
        c = clamp_cursor_to_lines(c, lines);
        return make_line_motion(win.cursor(), c, static_cast<int>(lines.size()));
    }

    inline MotionResult line_start(Window &win)
    {
        Cursor c = win.cursor();
        c.column = 0;
        win.preferred_column() = 0;
        return make_char_motion(win.cursor(), c, win.buffer().lines());
    }

    inline MotionResult first_non_blank(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c = win.cursor();
        c.column = 0;
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(c.line)];
            std::size_t i = 0;
            while (i < row.size())
            {
                const auto [cp, n] = TextMetrics::decode(row, i);
                if (n == 0)
                    break;
                if (!is_space_cp(cp))
                    break;
                i += n;
            }
            c.column = static_cast<int>(i);
            set_preferred_display(win, row, c.column);
        }
        else
        {
            win.preferred_column() = 0;
        }
        return make_char_motion(win.cursor(), c, lines);
    }

    inline MotionResult line_end(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c = win.cursor();
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(c.line)];
            if (row.empty())
            {
                c.column = 0;
            }
            else
            {
                // Last codepoint start.
                c.column = static_cast<int>(TextMetrics::prev_cp(row, row.size()));
            }
            set_preferred_display(win, row, c.column);
        }
        MotionResult r = make_char_motion(win.cursor(), c, lines);
        if (!lines.empty())
        {
            const int len = static_cast<int>(lines[static_cast<std::size_t>(c.line)].size());
            if (len > 0)
            {
                Cursor from = clamp_cursor_to_lines(win.cursor(), lines);
                r.range.start = from;
                r.range.end = {.line = c.line, .column = len};
                if (cursor_before(r.range.end, r.range.start))
                    std::swap(r.range.start, r.range.end);
            }
        }
        return r;
    }

    inline MotionResult file_start(Window &win)
    {
        Cursor c{.line = 0, .column = 0};
        win.preferred_column() = 0;
        return make_line_motion(win.cursor(), c, static_cast<int>(win.buffer().lines().size()));
    }

    inline MotionResult file_end(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c{.line = 0, .column = 0};
        if (!lines.empty())
            c.line = static_cast<int>(lines.size()) - 1;
        c.column = 0;
        win.preferred_column() = 0;
        return make_line_motion(win.cursor(), c, static_cast<int>(lines.size()));
    }

    inline MotionResult word_forward(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c = clamp_cursor_to_lines(win.cursor(), lines);
        if (lines.empty())
            return make_char_motion(c, c, lines);

        auto decode_at = [](const std::string &row, int byte) -> std::pair<char32_t, std::size_t> {
            return TextMetrics::decode(row, static_cast<std::size_t>(byte));
        };

        auto &row = lines[static_cast<std::size_t>(c.line)];
        if (c.column < static_cast<int>(row.size()))
        {
            const auto [cp0, n0] = decode_at(row, c.column);
            if (n0 > 0 && is_word_cp(cp0))
            {
                while (c.column < static_cast<int>(row.size()))
                {
                    const auto [cp, n] = decode_at(row, c.column);
                    if (n == 0 || !is_word_cp(cp))
                        break;
                    c.column += static_cast<int>(n);
                }
            }
            else if (n0 > 0 && !is_space_cp(cp0))
            {
                while (c.column < static_cast<int>(row.size()))
                {
                    const auto [cp, n] = decode_at(row, c.column);
                    if (n == 0 || is_space_cp(cp) || is_word_cp(cp))
                        break;
                    c.column += static_cast<int>(n);
                }
            }
        }

        while (true)
        {
            auto &cur = lines[static_cast<std::size_t>(c.line)];
            while (c.column < static_cast<int>(cur.size()))
            {
                const auto [cp, n] = decode_at(cur, c.column);
                if (n == 0 || !is_space_cp(cp))
                    break;
                c.column += static_cast<int>(n);
            }
            if (c.column < static_cast<int>(cur.size()))
                break;
            if (c.line + 1 >= static_cast<int>(lines.size()))
                break;
            c.line++;
            c.column = 0;
        }

        set_preferred_display(win, lines[static_cast<std::size_t>(c.line)], c.column);
        return make_char_motion(win.cursor(), c, lines);
    }

    inline MotionResult word_backward(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c = clamp_cursor_to_lines(win.cursor(), lines);
        if (lines.empty())
            return make_char_motion(c, c, lines);

        auto step_back = [&]() {
            if (c.column > 0)
            {
                const auto &row = lines[static_cast<std::size_t>(c.line)];
                c.column = static_cast<int>(
                    TextMetrics::prev_cp(row, static_cast<std::size_t>(c.column)));
                return true;
            }
            if (c.line > 0)
            {
                c.line--;
                const auto &row = lines[static_cast<std::size_t>(c.line)];
                c.column = static_cast<int>(row.size());
                if (c.column > 0)
                    c.column = static_cast<int>(TextMetrics::prev_cp(row, static_cast<std::size_t>(c.column)));
                return true;
            }
            return false;
        };

        auto cp_here = [&]() -> char32_t {
            const auto &row = lines[static_cast<std::size_t>(c.line)];
            if (row.empty() || c.column >= static_cast<int>(row.size()))
                return U' ';
            return TextMetrics::decode(row, static_cast<std::size_t>(c.column)).first;
        };

        if (!step_back())
            return make_char_motion(win.cursor(), c, lines);

        while (is_space_cp(cp_here()))
        {
            const auto &row = lines[static_cast<std::size_t>(c.line)];
            if (row.empty() || c.column >= static_cast<int>(row.size()))
            {
                if (!step_back())
                    break;
                continue;
            }
            if (!is_space_cp(cp_here()))
                break;
            if (!step_back())
                break;
        }

        if (lines[static_cast<std::size_t>(c.line)].empty())
        {
            set_preferred_display(win, lines[static_cast<std::size_t>(c.line)], c.column);
            return make_char_motion(win.cursor(), c, lines);
        }

        const auto &row = lines[static_cast<std::size_t>(c.line)];
        if (c.column >= static_cast<int>(row.size()))
            c.column = static_cast<int>(TextMetrics::prev_cp(row, row.size()));

        const char32_t start_cp = cp_here();
        if (is_word_cp(start_cp))
        {
            while (c.column > 0)
            {
                const std::size_t prev = TextMetrics::prev_cp(row, static_cast<std::size_t>(c.column));
                const auto [cp, n] = TextMetrics::decode(row, prev);
                (void)n;
                if (!is_word_cp(cp))
                    break;
                c.column = static_cast<int>(prev);
            }
        }
        else if (!is_space_cp(start_cp))
        {
            while (c.column > 0)
            {
                const std::size_t prev = TextMetrics::prev_cp(row, static_cast<std::size_t>(c.column));
                const auto [cp, n] = TextMetrics::decode(row, prev);
                (void)n;
                if (is_space_cp(cp) || is_word_cp(cp))
                    break;
                c.column = static_cast<int>(prev);
            }
        }

        set_preferred_display(win, row, c.column);
        return make_char_motion(win.cursor(), c, lines);
    }

    inline MotionResult word_end(Window &win)
    {
        const auto &lines = win.buffer().lines();
        Cursor c = clamp_cursor_to_lines(win.cursor(), lines);
        if (lines.empty())
            return make_char_motion(c, c, lines);

        auto at_end = [&]() {
            const auto &row = lines[static_cast<std::size_t>(c.line)];
            return c.line + 1 >= static_cast<int>(lines.size()) &&
                   c.column >= static_cast<int>(row.size());
        };

        if (c.column < static_cast<int>(lines[static_cast<std::size_t>(c.line)].size()))
            c.column = static_cast<int>(TextMetrics::next_cp(
                lines[static_cast<std::size_t>(c.line)], static_cast<std::size_t>(c.column)));

        while (!at_end())
        {
            auto &row = lines[static_cast<std::size_t>(c.line)];
            while (c.column < static_cast<int>(row.size()))
            {
                const auto [cp, n] = TextMetrics::decode(row, static_cast<std::size_t>(c.column));
                if (n == 0 || !is_space_cp(cp))
                    break;
                c.column += static_cast<int>(n);
            }
            if (c.column >= static_cast<int>(row.size()))
            {
                if (c.line + 1 >= static_cast<int>(lines.size()))
                    break;
                c.line++;
                c.column = 0;
                continue;
            }

            const auto [cp0, n0] = TextMetrics::decode(row, static_cast<std::size_t>(c.column));
            if (n0 > 0 && is_word_cp(cp0))
            {
                while (true)
                {
                    const std::size_t nxt = TextMetrics::next_cp(row, static_cast<std::size_t>(c.column));
                    if (nxt >= row.size())
                        break;
                    const auto [cp, n] = TextMetrics::decode(row, nxt);
                    if (n == 0 || !is_word_cp(cp))
                        break;
                    c.column = static_cast<int>(nxt);
                }
                break;
            }

            while (true)
            {
                const std::size_t nxt = TextMetrics::next_cp(row, static_cast<std::size_t>(c.column));
                if (nxt >= row.size())
                    break;
                const auto [cp, n] = TextMetrics::decode(row, nxt);
                if (n == 0 || is_space_cp(cp) || is_word_cp(cp))
                    break;
                c.column = static_cast<int>(nxt);
            }
            break;
        }

        set_preferred_display(win, lines[static_cast<std::size_t>(c.line)], c.column);
        MotionResult r = make_char_motion(win.cursor(), c, lines);
        const auto &row = lines[static_cast<std::size_t>(c.line)];
        const int len = static_cast<int>(row.size());
        if (c.column < len)
            r.range.end = {
                .line = c.line,
                .column = static_cast<int>(
                    TextMetrics::next_cp(row, static_cast<std::size_t>(c.column)))};
        return r;
    }
}
