#pragma once
#include <editor/motion_types.hpp>

namespace Motion
{
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

}
