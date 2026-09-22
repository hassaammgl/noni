#include "editor_impl.hpp"

void Editor::draw_window_pane(Window &w, int ox, int oy, int pw, int ph, bool is_active)
{
    if (!window || !w.has_buffer() || pw <= 0 || ph <= 0)
        return;

    // Clamp scroll for this pane size.
    if (w.cursor().line < w.scroll_y())
        w.scroll_y() = w.cursor().line;
    if (w.cursor().line >= w.scroll_y() + ph)
        w.scroll_y() = w.cursor().line - ph + 1;
    if (w.scroll_y() < 0)
        w.scroll_y() = 0;
    sync_horizontal_scroll(w, pw);

    const auto &content = w.buffer().lines();
    w.buffer().sync_syntax();
    auto &syntax = w.buffer().syntax();

    const int sx = w.scroll_x();
    const int sy = w.scroll_y();

    for (int row = 0; row < ph; ++row)
    {
        const int line_index = sy + row;
        if (line_index >= static_cast<int>(content.size()))
            continue;

        const std::string &line = content[static_cast<std::size_t>(line_index)];
        const auto &tokens = syntax.tokens_for_line(line_index);
        const int line_len = static_cast<int>(line.size());

        paint_byte_range(
            window, oy + row, ox, line, 0, line_len, sx, pw, Theme::Editor);

        for (const auto &tok : tokens)
        {
            if (tok.kind == TokenKind::Text)
                continue;
            const int tok_start = tok.start;
            const int tok_end = std::min(tok.start + tok.length, line_len);
            if (tok_start >= tok_end)
                continue;
            paint_byte_range(
                window,
                oy + row,
                ox,
                line,
                tok_start,
                tok_end,
                sx,
                pw,
                Syntax::theme_pair(tok.kind));
        }
    }

    // Precedence: base syntax → search → selection → diagnostics → cursor.
    draw_search_highlight(w, ox, oy, pw, ph);
    draw_selection_overlay(w, ox, oy, pw, ph);
    draw_diagnostics_overlay(w, ox, oy, pw, ph);

    if (is_active)
    {
        const int cy = w.cursor().line - w.scroll_y();
        const int cx = cursor_display_col(w) - w.scroll_x();
        if (cy >= 0 && cy < ph && cx >= 0 && cx < pw)
            wmove(window, oy + cy, ox + cx);
    }
}

void Editor::draw()
{
    if (!window || !tab)
        return;

    werase(window);
    leaveok(window, FALSE);
    wbkgd(window, COLOR_PAIR(Theme::Editor));
    wattron(window, COLOR_PAIR(Theme::Editor));
    for (int row = 0; row < height; ++row)
        mvwhline(window, row, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::Editor));

    clamp_cursor();
    update_scroll();

    if (core_)
        core_->set_editor_area(width, height);

    auto rects = tab->layout.compute_rects(0, 0, width, height);
    Window *active = tab->layout.active();

    // Draw split separators.
    for (const auto &[w, r] : rects)
    {
        (void)w;
        // Vertical separator to the right of pane if another pane starts at x+width+1
        for (const auto &[w2, r2] : rects)
        {
            if (r2.x == r.x + r.width + 1 && r2.y < r.y + r.height && r.y < r2.y + r2.height)
            {
                wattron(window, COLOR_PAIR(Theme::Border));
                for (int y = std::max(r.y, r2.y); y < std::min(r.y + r.height, r2.y + r2.height); ++y)
                    mvwaddch(window, y, r.x + r.width, ACS_VLINE);
                wattroff(window, COLOR_PAIR(Theme::Border));
            }
            if (r2.y == r.y + r.height + 1 && r2.x < r.x + r.width && r.x < r2.x + r2.width)
            {
                wattron(window, COLOR_PAIR(Theme::Border));
                mvwhline(window, r.y + r.height, r.x, ACS_HLINE, r.width);
                wattroff(window, COLOR_PAIR(Theme::Border));
            }
        }
    }

    for (auto &[w, r] : rects)
    {
        if (!w)
            continue;
        draw_window_pane(*w, r.x, r.y, r.width, r.height, w == active);
    }
}
