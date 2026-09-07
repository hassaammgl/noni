#include <components/line_number.hpp>
#include <ui/theme.hpp>

void LineNumber::sync(int scroll, int active, int total)
{
    scroll_y = scroll;
    active_line = active;
    total_lines = total;
}

void LineNumber::draw()
{
    if (!window)
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Editor));

    for (int row = 0; row < height; ++row)
    {
        const int line_index = scroll_y + row;
        if (line_index >= total_lines)
            break;

        const short pair = (line_index == active_line)
                               ? Theme::LineNumberActive
                               : Theme::LineNumber;

        wattron(window, COLOR_PAIR(pair));
        mvwprintw(window, row, 0, "%*d", width - 1, line_index + 1);
        wattroff(window, COLOR_PAIR(pair));
    }
}
