#include "components/statusbar.hpp"

void Statusbar::draw()
{
    if (!window)
        return;

    werase(window);

    wbkgd(window, COLOR_PAIR(2));

    mvwprintw(
        window,
        0,
        1,
        " NORMAL ");

    mvwprintw(
        window,
        0,
        15,
        "Ln %d, Col %d", cursor.line, cursor.column);

    mvwprintw(
        window,
        0,
        30,
        "UTF-8");

    int filenameX =
        width - static_cast<int>(this->filename.length()) - 2;

    if (filenameX < 0)
        filenameX = 0;

    mvwprintw(
        window,
        0,
        filenameX,
        "%s",
        this->filename.c_str());
}

void Statusbar::setMode(const std::string &mode)
{
    this->mode = mode;
}

void Statusbar::setFilename(const std::string &filename)
{
    this->filename = filename;
}

void Statusbar::setCursorPosition(int line, int column)
{
    this->cursor = {.line = line, .column = column};
}
