#include <components/statusbar.hpp>
#include <ui/theme.hpp>
#include <algorithm>
#include <format>

void Statusbar::draw()
{
    if (!window)
        return;

    // Expire ephemeral echo without logging.
    if (!echo_.empty() && std::chrono::steady_clock::now() >= echo_until_)
        echo_.clear();

    werase(window);
    leaveok(window, TRUE);

    wbkgd(window, COLOR_PAIR(Theme::Statusbar));
    wattron(window, COLOR_PAIR(Theme::Statusbar));
    mvwhline(window, 0, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::Statusbar));

    short mode_pair = Theme::StatusbarModeNormal;
    if (mode == "INSERT")
        mode_pair = Theme::StatusbarModeInsert;
    else if (mode == "VISUAL" || mode == "V-LINE")
        mode_pair = Theme::StatusbarModeVisual;
    else if (mode == "COMMAND")
        mode_pair = Theme::InputFocus;
    else if (mode == "SIDEBAR")
        mode_pair = Theme::StatusbarDebugging;
    else if (mode == "MESSAGES")
        mode_pair = Theme::Notification;

    wattron(window, COLOR_PAIR(mode_pair));
    mvwprintw(window, 0, 1, " %s ", this->mode.c_str());
    wattroff(window, COLOR_PAIR(mode_pair));

    wattron(window, COLOR_PAIR(Theme::Statusbar));
    mvwprintw(window, 0, 15, "Ln %d, Chr %d, Col %d", line_, char_, display_col_);

    if (!scm_badge_.empty())
        mvwprintw(window, 0, 42, "Git %s", scm_badge_.c_str());
    else
        mvwprintw(window, 0, 42, "UTF-8");

    // Echo sits between cursor info and filename — clipped so filename stays visible.
    if (!echo_.empty())
    {
        const int filename_room = static_cast<int>(filename.length()) + 3;
        const int echo_x = 52;
        int echo_w = width - echo_x - filename_room;
        if (echo_w > 8)
        {
            std::string shown = echo_;
            if (static_cast<int>(shown.size()) > echo_w)
                shown = shown.substr(0, static_cast<std::size_t>(echo_w - 1)) + "…";
            wattron(window, COLOR_PAIR(Theme::Notification));
            mvwprintw(window, 0, echo_x, "%s", shown.c_str());
            wattroff(window, COLOR_PAIR(Theme::Notification));
        }
    }

    int filename_x =
        width - static_cast<int>(this->filename.length()) - 2;

    if (filename_x < 0)
        filename_x = 0;

    wattron(window, COLOR_PAIR(Theme::Statusbar));
    mvwprintw(window, 0, filename_x, "%s", this->filename.c_str());
    wattroff(window, COLOR_PAIR(Theme::Statusbar));
}

void Statusbar::set_mode(const std::string &mode)
{
    this->mode = mode;
}

void Statusbar::set_filename(const std::string &filename)
{
    this->filename = filename;
}

void Statusbar::set_cursor_position(int line, int char_pos, int display_col)
{
    line_ = line;
    char_ = char_pos;
    display_col_ = display_col;
}

void Statusbar::set_scm_badge(std::string badge)
{
    scm_badge_ = std::move(badge);
}

void Statusbar::set_echo(std::string text, int ttl_ms)
{
    echo_ = std::move(text);
    echo_until_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(500, ttl_ms));
}

void Statusbar::clear_echo()
{
    echo_.clear();
}
