#pragma once
#include <editor/motion_types.hpp>

namespace Motion
{
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

}
