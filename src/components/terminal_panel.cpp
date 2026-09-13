#include <components/terminal_panel.hpp>
#include <ui/theme.hpp>

#include <algorithm>
#include <cctype>

namespace
{
    bool is_csi_final(char c)
    {
        return c >= 0x40 && c <= 0x7e;
    }
}

int TerminalPanel::view_rows() const
{
    return std::max(0, height - 1);
}

void TerminalPanel::ensure_line()
{
    if (lines.empty())
        lines.push_back("");
}

void TerminalPanel::newline()
{
    lines.push_back("");
    cursor_col = 0;
    while (static_cast<int>(lines.size()) > kMaxLines)
        lines.erase(lines.begin());
}

void TerminalPanel::put_char(char ch)
{
    ensure_line();
    auto &line = lines.back();
    if (cursor_col < 0)
        cursor_col = 0;
    if (cursor_col > static_cast<int>(line.size()))
        cursor_col = static_cast<int>(line.size());

    if (cursor_col == static_cast<int>(line.size()))
        line.push_back(ch);
    else
        line[static_cast<std::size_t>(cursor_col)] = ch;
    ++cursor_col;

    // Soft wrap against panel width (if known).
    if (width > 2 && cursor_col >= width)
        newline();
}

void TerminalPanel::strip_and_put(const std::string &chunk)
{
    for (std::size_t i = 0; i < chunk.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(chunk[i]);

        if (c == '\n')
        {
            newline();
            continue;
        }
        if (c == '\r')
        {
            cursor_col = 0;
            continue;
        }
        if (c == '\b')
        {
            if (cursor_col > 0)
                --cursor_col;
            continue;
        }
        if (c == '\t')
        {
            const int next = ((cursor_col / 8) + 1) * 8;
            while (cursor_col < next)
                put_char(' ');
            continue;
        }
        if (c == 0x1b)
        {
            // ESC [ ... final  or ESC ] ... BEL  or ESC single-char
            if (i + 1 >= chunk.size())
                break;
            const char next = chunk[i + 1];
            if (next == '[')
            {
                i += 2;
                while (i < chunk.size() && !is_csi_final(chunk[i]))
                    ++i;
                continue;
            }
            if (next == ']')
            {
                i += 2;
                while (i < chunk.size() && chunk[i] != '\a' && chunk[i] != 0x1b)
                    ++i;
                continue;
            }
            // Skip ESC + one char
            ++i;
            continue;
        }
        if (c < 32)
            continue;

        put_char(static_cast<char>(c));
    }
}

void TerminalPanel::ingest(const std::string &chunk)
{
    if (chunk.empty())
        return;
    const bool follow = (scroll_back == 0);
    strip_and_put(chunk);
    if (follow)
        scroll_back = 0;
}

void TerminalPanel::draw()
{
    if (!window || !visible || height <= 0 || width <= 0)
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Editor));

    // Title
    const short title = focused ? Theme::SidebarSelected : Theme::SidebarTitle;
    wattron(window, COLOR_PAIR(title));
    mvwhline(window, 0, 0, ' ', width);
    mvwprintw(window, 0, 1, " TERMINAL ");
    if (focused)
        mvwprintw(window, 0, 12, "FOCUS");
    if (!pty.alive())
        mvwprintw(window, 0, std::max(1, width - 10), "exited");
    wattroff(window, COLOR_PAIR(title));

    const int rows = view_rows();
    if (rows <= 0)
        return;

    const int total = static_cast<int>(lines.size());
    int start = std::max(0, total - rows - scroll_back);
    if (start + rows > total)
        start = std::max(0, total - rows);

    wattron(window, COLOR_PAIR(Theme::Editor));
    for (int r = 0; r < rows; ++r)
    {
        const int idx = start + r;
        mvwhline(window, r + 1, 0, ' ', width);
        if (idx < 0 || idx >= total)
            continue;
        const std::string &line = lines[static_cast<std::size_t>(idx)];
        mvwprintw(window, r + 1, 0, "%.*s", width, line.c_str());
    }
    wattroff(window, COLOR_PAIR(Theme::Editor));

    if (focused && scroll_back == 0 && !lines.empty())
    {
        const int cy = height - 1;
        const int cx = std::min(width - 1, std::max(0, cursor_col));
        wmove(window, cy, cx);
        leaveok(window, FALSE);
    }
}

void TerminalPanel::set_visible(bool v)
{
    visible = v;
}

bool TerminalPanel::is_visible() const
{
    return visible;
}

void TerminalPanel::set_focused(bool v)
{
    focused = v;
}

bool TerminalPanel::is_focused() const
{
    return focused;
}

void TerminalPanel::set_cwd(const std::string &path)
{
    cwd = path;
}

void TerminalPanel::ensure_started()
{
    if (pty.alive())
        return;
    const int rows = std::max(1, view_rows());
    const int cols = std::max(1, width > 0 ? width : 80);
    lines.assign(1, "");
    cursor_col = 0;
    scroll_back = 0;
    pty.start(rows, cols, cwd);
}

void TerminalPanel::stop()
{
    pty.stop();
}

bool TerminalPanel::poll()
{
    if (!visible)
        return false;
    const std::string chunk = pty.take_output();
    if (chunk.empty())
        return false;
    ingest(chunk);
    return true;
}

void TerminalPanel::on_resized()
{
    if (!visible || !pty.alive())
        return;
    pty.resize(std::max(1, view_rows()), std::max(1, width));
}

void TerminalPanel::handle_input(int key)
{
    if (!visible)
        return;

    if (!pty.alive())
        ensure_started();

    // Scrollback navigation (doesn't go to shell)
    if (key == KEY_PPAGE)
    {
        scroll_back = std::min(
            std::max(0, static_cast<int>(lines.size()) - view_rows()),
            scroll_back + std::max(1, view_rows()));
        return;
    }
    if (key == KEY_NPAGE)
    {
        scroll_back = std::max(0, scroll_back - std::max(1, view_rows()));
        return;
    }

    // Typing jumps back to live edge
    scroll_back = 0;

    auto send = [&](const char *s, std::size_t n) { pty.write_bytes(s, n); };
    auto send1 = [&](char c) { pty.write_byte(c); };

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
    default:
        if (key >= 1 && key <= 26)
        {
            // Ctrl+A .. Ctrl+Z → shell
            send1(static_cast<char>(key));
            break;
        }
        if (key >= 32 && key <= 126)
            send1(static_cast<char>(key));
        break;
    }
}
