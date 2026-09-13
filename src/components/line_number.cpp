#include <components/line_number.hpp>
#include <ui/theme.hpp>

#include <algorithm>

std::optional<DiagnosticSeverity> LineNumber::worst_on_line(const DiagnosticSnapshot *diags, int line)
{
    if (!diags)
        return std::nullopt;
    std::optional<DiagnosticSeverity> worst;
    for (const auto &d : diags->items)
    {
        const int lo = std::min(d.start.line, d.end.line);
        const int hi = std::max(d.start.line, d.end.line);
        if (line < lo || line > hi)
            continue;
        if (!worst || static_cast<int>(d.severity) < static_cast<int>(*worst))
            worst = d.severity;
    }
    return worst;
}

void LineNumber::sync(int scroll, int active, int total, const DiagnosticSnapshot *diagnostics)
{
    scroll_y = scroll;
    active_line = active;
    total_lines = total;
    diagnostics_ = diagnostics;
}

void LineNumber::draw()
{
    if (!window)
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::LineNumber));
    wattron(window, COLOR_PAIR(Theme::LineNumber));
    for (int row = 0; row < height; ++row)
        mvwhline(window, row, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::LineNumber));

    for (int row = 0; row < height; ++row)
    {
        const int line_index = scroll_y + row;
        if (line_index >= total_lines)
            continue;

        const short pair = (line_index == active_line)
                               ? Theme::LineNumberActive
                               : Theme::LineNumber;

        wattron(window, COLOR_PAIR(pair));
        const int num_width = std::max(1, width - 2);
        mvwprintw(window, row, 0, "%*d", num_width, line_index + 1);
        wattroff(window, COLOR_PAIR(pair));

        const auto sev = worst_on_line(diagnostics_, line_index);
        if (!sev)
            continue;

        short mark_pair = Theme::Hint;
        char mark = '.';
        switch (*sev)
        {
        case DiagnosticSeverity::Error:
            mark_pair = Theme::Error;
            mark = '!';
            break;
        case DiagnosticSeverity::Warning:
            mark_pair = Theme::Warning;
            mark = '?';
            break;
        case DiagnosticSeverity::Information:
            mark_pair = Theme::Info;
            mark = 'i';
            break;
        case DiagnosticSeverity::Hint:
            mark_pair = Theme::Hint;
            mark = '.';
            break;
        }
        wattron(window, COLOR_PAIR(mark_pair));
        mvwaddch(window, row, width - 1, mark);
        wattroff(window, COLOR_PAIR(mark_pair));
    }
}
