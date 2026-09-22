#include "editor_impl.hpp"

void Editor::draw_selection_overlay(Window &w, int ox, int oy, int pw, int ph)
{
    if (!w.has_selection() || !window || pw <= 0 || ph <= 0)
        return;

    const auto &content = w.buffer().lines();
    const int sx = w.scroll_x();
    const int sy = w.scroll_y();

    for (int row = 0; row < ph; ++row)
    {
        const int line_index = sy + row;
        if (line_index < 0 || line_index >= static_cast<int>(content.size()))
            continue;

        const std::string &line = content[static_cast<std::size_t>(line_index)];

        if (w.selection().kind == SelectionKind::Line)
        {
            const int lo = std::min(w.selection().anchor.line, w.cursor().line);
            const int hi = std::max(w.selection().anchor.line, w.cursor().line);
            if (line_index < lo || line_index > hi)
                continue;

            paint_byte_range(
                window,
                oy + row,
                ox,
                line,
                0,
                static_cast<int>(line.size()),
                sx,
                pw,
                Theme::Selection);
            // Fill remainder of pane past EOL.
            const int line_dw = TextMetrics::line_display_width(line);
            if (line_dw < sx + pw)
            {
                const int fill_start = std::max(0, line_dw - sx);
                wattron(window, COLOR_PAIR(Theme::Selection));
                mvwhline(window, oy + row, ox + fill_start, ' ', pw - fill_start);
                wattroff(window, COLOR_PAIR(Theme::Selection));
            }
            continue;
        }

        // Character selection: paint selected codepoints by byte range.
        int dcol = 0;
        std::size_t i = 0;
        while (i < line.size() && dcol < sx + pw)
        {
            const auto [cp, n] = TextMetrics::decode(line, i);
            if (n == 0)
                break;
            if (w.is_selected(line_index, static_cast<int>(i)))
            {
                paint_byte_range(
                    window,
                    oy + row,
                    ox,
                    line,
                    static_cast<int>(i),
                    static_cast<int>(i + n),
                    sx,
                    pw,
                    Theme::Selection);
            }
            dcol += cp_display_width(cp, dcol);
            i += n;
        }
        // Visual EOL cell when selection includes end.
        const int eol_disp = TextMetrics::line_display_width(line);
        if (w.is_selected(line_index, static_cast<int>(line.size())) &&
            eol_disp >= sx && eol_disp < sx + pw)
        {
            wattron(window, COLOR_PAIR(Theme::Selection));
            mvwaddch(window, oy + row, ox + eol_disp - sx, ' ');
            wattroff(window, COLOR_PAIR(Theme::Selection));
        }
    }
}

void Editor::draw_search_highlight(Window &w, int ox, int oy, int pw, int ph)
{
    if (!core_ || !window || pw <= 0 || ph <= 0)
        return;

    const auto &st = core_->search();
    if (!st.active() || st.matches().empty())
        return;
    if (!w.has_buffer())
        return;
    if (st.buffer_id() != reinterpret_cast<std::uintptr_t>(&w.buffer()))
        return;

    const auto &content = w.buffer().lines();
    const int sx = w.scroll_x();
    const int sy = w.scroll_y();
    const int cur_idx = st.current_index();

    for (int mi = 0; mi < static_cast<int>(st.matches().size()); ++mi)
    {
        const auto &m = st.matches()[static_cast<std::size_t>(mi)];
        if (m.start.line != m.end.line)
            continue;
        if (m.start.line < sy || m.start.line >= sy + ph)
            continue;

        const int row = m.start.line - sy;
        if (m.start.line >= static_cast<int>(content.size()))
            continue;
        const std::string &line = content[static_cast<std::size_t>(m.start.line)];
        const short pair = (mi == cur_idx) ? Theme::SearchMatchCurrent : Theme::SearchMatch;
        paint_byte_range(
            window,
            oy + row,
            ox,
            line,
            m.start.column,
            m.end.column,
            sx,
            pw,
            pair);
    }
}

void Editor::draw_diagnostics_overlay(Window &w, int ox, int oy, int pw, int ph)
{
    if (!window || !w.has_buffer() || pw <= 0 || ph <= 0)
        return;

    const auto &diags = w.buffer().diagnostics().items;
    if (diags.empty())
        return;

    const auto &content = w.buffer().lines();
    const int sx = w.scroll_x();
    const int sy = w.scroll_y();

    auto theme_for = [](DiagnosticSeverity s) -> short {
        switch (s)
        {
        case DiagnosticSeverity::Error:
            return Theme::Error;
        case DiagnosticSeverity::Warning:
            return Theme::Warning;
        case DiagnosticSeverity::Information:
            return Theme::Info;
        case DiagnosticSeverity::Hint:
            return Theme::Hint;
        }
        return Theme::Error;
    };

    for (const auto &d : diags)
    {
        const short pair = theme_for(d.severity);
        Cursor a = d.start;
        Cursor b = d.end;
        if (b.line < a.line || (b.line == a.line && b.column < a.column))
            std::swap(a, b);

        for (int line = a.line; line <= b.line; ++line)
        {
            if (line < sy || line >= sy + ph)
                continue;
            if (line < 0 || line >= static_cast<int>(content.size()))
                continue;

            const std::string &row = content[static_cast<std::size_t>(line)];
            const int line_len = static_cast<int>(row.size());
            int lo = (line == a.line) ? a.column : 0;
            int hi = (line == b.line) ? b.column : line_len;
            lo = std::clamp(lo, 0, line_len);
            hi = std::clamp(hi, 0, line_len);
            if (lo >= hi)
            {
                if (lo < line_len)
                {
                    const auto [cp, n] = TextMetrics::decode(row, static_cast<std::size_t>(lo));
                    (void)cp;
                    hi = lo + std::max(1, static_cast<int>(n));
                }
                else if (line_len > 0)
                {
                    lo = line_len - 1;
                    hi = line_len;
                }
                else
                    continue;
            }
            paint_byte_range(
                window, oy + (line - sy), ox, row, lo, hi, sx, pw, pair);
        }
    }
}

