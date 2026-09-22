#pragma once
#include <editor/motion_types.hpp>

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

}
