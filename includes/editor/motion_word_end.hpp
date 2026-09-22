#pragma once
#include <editor/motion_types.hpp>

namespace Motion
{
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
