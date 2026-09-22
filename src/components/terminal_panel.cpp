#include <components/terminal_panel.hpp>
#include <ui/theme.hpp>

#include <algorithm>
#include <ncurses.h>
#include <vector>

#include "terminal_panel_detail.hpp"

using namespace terminal_panel_detail;

int TerminalPanel::view_rows() const
{
    return std::max(0, height - 1);
}

short TerminalPanel::color_for_cell(const TerminalCell &cell) const
{
    TerminalAttrs a = cell.attrs;
    if (a.inverse)
        std::swap(a.fg, a.bg);

    auto map_fg = [](std::uint8_t fg) -> short {
        switch (fg)
        {
        case 0:
            return Theme::Dim;
        case 1:
            return Theme::Error;
        case 2:
            return Theme::GitAdded;
        case 3:
            return Theme::Warning;
        case 4:
            return Theme::Info;
        case 5:
            return Theme::Macro;
        case 6:
            return Theme::Constant;
        case 7:
            return Theme::Editor;
        case 8:
            return Theme::Dim;
        case 9:
            return Theme::Error;
        case 10:
            return Theme::GitAdded;
        case 11:
            return Theme::Warning;
        case 12:
            return Theme::Info;
        case 13:
            return Theme::Macro;
        case 14:
            return Theme::String;
        case 15:
            return Theme::Editor;
        default:
            return Theme::Editor;
        }
    };

    short pair = Theme::Editor;
    if (a.fg != 255)
        pair = map_fg(a.fg);
    else if (a.bold)
        pair = Theme::Keyword;
    else if (a.dim)
        pair = Theme::Dim;
    if (a.underline)
        pair = Theme::MarkupLink;
    return pair;
}

void TerminalPanel::draw_cell(int row, int col, const TerminalCell &cell)
{
    if (!window || cell.width == 0)
        return;
    if (col < 0 || col >= width || row < 0 || row >= height)
        return;

    const short pair = color_for_cell(cell);
    wattron(window, COLOR_PAIR(pair));
    if (cell.attrs.bold)
        wattron(window, A_BOLD);
    if (cell.attrs.underline)
        wattron(window, A_UNDERLINE);

    if (cell.ch == 0 || cell.ch == U' ')
    {
        mvwaddch(window, row, col, ' ');
    }
    else
    {
        char buf[8];
        int len = 0;
        encode_utf8(cell.ch, buf, len);
        mvwaddnstr(window, row, col, buf, len);
    }

    if (cell.attrs.underline)
        wattroff(window, A_UNDERLINE);
    if (cell.attrs.bold)
        wattroff(window, A_BOLD);
    wattroff(window, COLOR_PAIR(pair));
}

void TerminalPanel::draw()
{
    if (!window || !visible_ || height <= 0 || width <= 0)
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Editor));

    const short title = focused_ ? Theme::SidebarSelected : Theme::SidebarTitle;
    wattron(window, COLOR_PAIR(title));
    mvwhline(window, 0, 0, ' ', width);
    mvwprintw(window, 0, 1, " TERMINAL ");
    if (focused_)
        mvwprintw(window, 0, 12, "FOCUS");

    const auto st = session_.state();
    if (st == TerminalSessionState::Exited)
        mvwprintw(window, 0, std::max(1, width - 10), "exited");
    else if (st == TerminalSessionState::Failed)
        mvwprintw(window, 0, std::max(1, width - 10), "failed");
    else if (session_.view_scroll() > 0)
        mvwprintw(window, 0, std::max(1, width - 12), "[scroll]");

    if (!session_.screen().title().empty() && width > 24)
    {
        std::string t = session_.screen().title();
        if (static_cast<int>(t.size()) > width - 24)
            t.resize(static_cast<std::size_t>(std::max(0, width - 24)));
        mvwprintw(window, 0, 20, "%s", t.c_str());
    }
    wattroff(window, COLOR_PAIR(title));

    const int rows = view_rows();
    if (rows <= 0)
        return;

    const auto &screen = session_.screen();
    const int total = screen.total_history_rows();
    const int scroll = session_.view_scroll();
    int start = std::max(0, total - rows - scroll);
    if (start + rows > total)
        start = std::max(0, total - rows);

    std::vector<TerminalCell> line;
    for (int r = 0; r < rows; ++r)
    {
        const int hist = start + r;
        screen.snapshot_view_line(hist, line);
        mvwhline(window, r + 1, 0, ' ', width);
        for (int c = 0; c < width && c < static_cast<int>(line.size()); ++c)
            draw_cell(r + 1, c, line[static_cast<std::size_t>(c)]);
    }

    if (focused_ && scroll == 0 && session_.alive())
    {
        const int cy = 1 + screen.cursor_row();
        const int cx = std::min(width - 1, std::max(0, screen.cursor_col()));
        if (cy >= 1 && cy < height)
        {
            wmove(window, cy, cx);
            leaveok(window, FALSE);
        }
    }
}

