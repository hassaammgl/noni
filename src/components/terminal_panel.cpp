#include <components/terminal_panel.hpp>
#include <ui/theme.hpp>

#include <algorithm>
#include <ncurses.h>
#include <vector>

namespace
{
    void encode_utf8(char32_t cp, char out[8], int &len)
    {
        len = 0;
        if (cp <= 0x7F)
        {
            out[len++] = static_cast<char>(cp);
        }
        else if (cp <= 0x7FF)
        {
            out[len++] = static_cast<char>(0xC0 | (cp >> 6));
            out[len++] = static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp <= 0xFFFF)
        {
            out[len++] = static_cast<char>(0xE0 | (cp >> 12));
            out[len++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out[len++] = static_cast<char>(0x80 | (cp & 0x3F));
        }
        else
        {
            out[len++] = static_cast<char>(0xF0 | (cp >> 18));
            out[len++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out[len++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out[len++] = static_cast<char>(0x80 | (cp & 0x3F));
        }
        out[len] = 0;
    }
}

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

void TerminalPanel::set_visible(bool v) { visible_ = v; }
bool TerminalPanel::is_visible() const { return visible_; }
void TerminalPanel::set_focused(bool v) { focused_ = v; }
bool TerminalPanel::is_focused() const { return focused_; }

void TerminalPanel::apply_config(const TerminalSessionConfig &cfg)
{
    session_.set_config(cfg);
}

void TerminalPanel::set_cwd(const std::string &path)
{
    auto cfg = session_.config();
    cfg.cwd = path;
    session_.set_config(cfg);
}

void TerminalPanel::ensure_started()
{
    if (session_.alive())
        return;
    const int rows = std::max(1, view_rows());
    const int cols = std::max(1, width > 0 ? width : 80);
    session_.start(rows, cols);
}

void TerminalPanel::stop()
{
    session_.stop();
}

bool TerminalPanel::poll()
{
    if (!visible_)
        return false;
    return session_.pump();
}

void TerminalPanel::on_resized()
{
    if (!visible_)
        return;
    const int rows = std::max(1, view_rows());
    const int cols = std::max(1, width);
    session_.resize(rows, cols);
}

void TerminalPanel::clear_screen()
{
    session_.clear_screen();
}

void TerminalPanel::scroll_up()
{
    session_.scroll_view(std::max(1, view_rows()));
}

void TerminalPanel::scroll_down()
{
    session_.scroll_view(-std::max(1, view_rows()));
}

void TerminalPanel::handle_input(int key)
{
    if (!visible_)
        return;

    if (!session_.alive())
        ensure_started();

    if (key == KEY_PPAGE)
    {
        scroll_up();
        return;
    }
    if (key == KEY_NPAGE)
    {
        scroll_down();
        return;
    }

    session_.follow_live();

    auto send = [&](const char *s, std::size_t n) { session_.write_bytes(s, n); };
    auto send1 = [&](char c) { session_.write_byte(c); };

    switch (key)
    {
    case KEY_ENTER:
    case '\n':
        send1('\r');
        break;
    case KEY_BACKSPACE:
    case 127:
    case 8:
        send1(127);
        break;
    case KEY_DC:
        send("\033[3~", 4);
        break;
    case KEY_UP:
        send("\033OA", 3);
        break;
    case KEY_DOWN:
        send("\033OB", 3);
        break;
    case KEY_RIGHT:
        send("\033OC", 3);
        break;
    case KEY_LEFT:
        send("\033OD", 3);
        break;
    case KEY_HOME:
        send("\033OH", 3);
        break;
    case KEY_END:
        send("\033OF", 3);
        break;
    case '\t':
        send1('\t');
        break;
    case 27:
        send1('\033');
        break;
    default:
        if (key >= 1 && key <= 26)
        {
            send1(static_cast<char>(key));
            break;
        }
        if (key >= 32 && key <= 126)
            send1(static_cast<char>(key));
        break;
    }
}
