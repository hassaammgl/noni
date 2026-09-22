#pragma once

#include <components/editor.hpp>
#include <lsp/diagnostics.hpp>
#include <syntax/syntax.hpp>
#include <ui/theme.hpp>
#include <utils/clipboard.hpp>
#include <utils/messages.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace editor_detail
{
    inline int cp_display_width(char32_t cp, int display_col)
    {
        if (cp == U'\t')
            return TextMetrics::tab_width_at(display_col);
        return TextMetrics::codepoint_width(cp);
    }

    inline int cursor_display_col(const Window &w)
    {
        if (!w.has_buffer())
            return 0;
        const auto &lines = w.buffer().lines();
        if (lines.empty())
            return 0;
        const Cursor &c = w.cursor();
        if (c.line < 0 || c.line >= static_cast<int>(lines.size()))
            return 0;
        return TextMetrics::byte_to_display(
            lines[static_cast<std::size_t>(c.line)], static_cast<std::size_t>(c.column));
    }

    inline void sync_preferred_display(Window &w)
    {
        w.preferred_column() = cursor_display_col(w);
    }

    inline void sync_horizontal_scroll(Window &w, int pane_w)
    {
        if (pane_w <= 0)
            return;
        const int dcol = cursor_display_col(w);
        if (dcol < w.scroll_x())
            w.scroll_x() = dcol;
        if (dcol >= w.scroll_x() + pane_w)
            w.scroll_x() = dcol - pane_w + 1;
        if (w.scroll_x() < 0)
            w.scroll_x() = 0;
    }

    // Paint UTF-8 codepoints whose byte range overlaps [byte_lo, byte_hi) into
    // display viewport [sx, sx + pw). Uses display columns for screen x.
    inline void paint_byte_range(
        WINDOW *win,
        int row,
        int ox,
        std::string_view line,
        int byte_lo,
        int byte_hi,
        int sx,
        int pw,
        short color_pair)
    {
        if (!win || pw <= 0 || byte_lo >= byte_hi)
            return;

        int dcol = 0;
        std::size_t i = 0;
        wattron(win, COLOR_PAIR(color_pair));
        while (i < line.size() && dcol < sx + pw)
        {
            const auto [cp, n] = TextMetrics::decode(line, i);
            if (n == 0)
                break;
            const int w = cp_display_width(cp, dcol);
            const int i_end = static_cast<int>(i + n);

            if (i_end > byte_lo && static_cast<int>(i) < byte_hi && dcol + w > sx)
            {
                if (cp == U'\t')
                {
                    for (int t = 0; t < w; ++t)
                    {
                        const int screen_x = dcol + t - sx;
                        if (screen_x >= 0 && screen_x < pw)
                            mvwaddch(win, row, ox + screen_x, ' ');
                    }
                }
                else
                {
                    int screen_x = dcol - sx;
                    if (screen_x < 0)
                    {
                        // Wide glyph straddling left edge: fill occupied cells.
                        for (int t = 0; t < w; ++t)
                        {
                            const int sx_cell = dcol + t - sx;
                            if (sx_cell >= 0 && sx_cell < pw)
                                mvwaddch(win, row, ox + sx_cell, ' ');
                        }
                    }
                    else if (screen_x < pw)
                    {
                        mvwaddnstr(
                            win,
                            row,
                            ox + screen_x,
                            line.data() + i,
                            static_cast<int>(n));
                    }
                }
            }

            dcol += w;
            i += n;
        }
        wattroff(win, COLOR_PAIR(color_pair));
    }
}

using editor_detail::cp_display_width;
using editor_detail::cursor_display_col;
using editor_detail::sync_preferred_display;
using editor_detail::sync_horizontal_scroll;
using editor_detail::paint_byte_range;

inline bool is_motion_key(int key)
{
    switch (key)
    {
    case 'h':
    case 'j':
    case 'k':
    case 'l':
    case 'w':
    case 'b':
    case 'e':
    case '0':
    case '^':
    case '$':
    case 'G':
    case KEY_UP:
    case KEY_DOWN:
    case KEY_LEFT:
    case KEY_RIGHT:
        return true;
    default:
        return false;
    }
}

inline MotionResult motion_from_key(Window &win, int key)
{
    switch (key)
    {
    case KEY_UP:
    case 'k':
        return Motion::up(win);
    case KEY_DOWN:
    case 'j':
        return Motion::down(win);
    case KEY_LEFT:
    case 'h':
        return Motion::left(win);
    case KEY_RIGHT:
    case 'l':
        return Motion::right(win);
    case 'w':
        return Motion::word_forward(win);
    case 'b':
        return Motion::word_backward(win);
    case 'e':
        return Motion::word_end(win);
    case '0':
        return Motion::line_start(win);
    case '^':
        return Motion::first_non_blank(win);
    case '$':
        return Motion::line_end(win);
    case 'G':
        return Motion::file_end(win);
    default:
        return {};
    }
}
