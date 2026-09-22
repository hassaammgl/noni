#pragma once
#include <editor/motion_types.hpp>

namespace Motion
{
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

}
