#include <components/statusbar.hpp>
#include <ui/theme.hpp>
#include <utils/logger.hpp>
#include <format>

void Statusbar::draw()
{
    if (!window)
        return;

    werase(window);
    leaveok(window, TRUE);

    wbkgd(window, COLOR_PAIR(Theme::Statusbar));

    short mode_pair = Theme::StatusbarModeNormal;
    if (mode == "INSERT")
        mode_pair = Theme::StatusbarModeInsert;
    else if (mode == "VISUAL")
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
    mvwprintw(window, 0, 15, "Ln %d, Col %d", cursor.line, cursor.column);

    mvwprintw(window, 0, 30, "UTF-8");

    int filename_x =
        width - static_cast<int>(this->filename.length()) - 2;

    if (filename_x < 0)
        filename_x = 0;

    mvwprintw(window, 0, filename_x, "%s", this->filename.c_str());
    wattroff(window, COLOR_PAIR(Theme::Statusbar));
}

void Statusbar::set_mode(const std::string &mode)
{
    if (this->mode != mode)
        Logger::debug(std::format("Statusbar mode: {} -> {}", this->mode, mode));
    this->mode = mode;
}

void Statusbar::set_filename(const std::string &filename)
{
    if (this->filename != filename)
        Logger::debug(std::format("Statusbar filename: {} -> {}", this->filename, filename));
    this->filename = filename;
}

void Statusbar::set_cursor_position(int line, int column)
{
    this->cursor = {.line = line, .column = column};
}
