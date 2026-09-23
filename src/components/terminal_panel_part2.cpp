#include <components/terminal_panel.hpp>
#include <configs/keybindings.hpp>
#include <ui/theme.hpp>

#include <algorithm>
#include <cstring>
#include <ncurses.h>
#include <vector>

#include "terminal_panel_detail.hpp"

using namespace terminal_panel_detail;

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
    case '\r':
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
    {
        if (const char *seq = KeybindingEngine::extended_seq(key))
        {
            send(seq, std::strlen(seq));
            break;
        }
        if (int n = 0; const char *seq = xterm_f_seq(key, n))
        {
            send(seq, static_cast<std::size_t>(n));
            break;
        }
        if (key >= 1 && key <= 26)
        {
            send1(static_cast<char>(key));
            break;
        }
        if (key >= 32 && key <= 126)
        {
            send1(static_cast<char>(key));
            break;
        }
        // 8-bit meta (Alt+key) when ncurses does not split ESC+key.
        if (key >= 128 && key < KEY_MIN)
        {
            send1('\033');
            send1(static_cast<char>(key & 0x7f));
            break;
        }
        break;
    }
    }
}

void TerminalPanel::insert_utf8(std::string_view utf8)
{
    if (!visible_ || utf8.empty())
        return;
    if (!session_.alive())
        ensure_started();
    session_.follow_live();
    session_.write_bytes(utf8.data(), utf8.size());
}
