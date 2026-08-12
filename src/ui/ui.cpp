#include "ui/ui.hpp"

UI::UI()
{
    header = nullptr;
    sidebar = nullptr;
    editor = nullptr;
    statusbar = nullptr;
}

UI::~UI()
{
    if (header)
        delwin(header);
    if (sidebar)
        delwin(sidebar);
    if (editor)
        delwin(editor);
    if (statusbar)
        delwin(statusbar);

    endwin();
}

void UI::init()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    resize();
}

void UI::resize()
{
    getmaxyx(stdscr, height, width);

    if (header)
        delwin(header);
    if (sidebar)
        delwin(sidebar);
    if (editor)
        delwin(editor);
    if (statusbar)
        delwin(statusbar);

    int contentHeight = height - 2;

    header = newwin(
        1,
        width,
        0,
        0);

    sidebar = newwin(
        contentHeight,
        sidebarWidth,
        1,
        0);

    editor = newwin(
        contentHeight,
        width - sidebarWidth,
        1,
        sidebarWidth);

    statusbar = newwin(
        1,
        width,
        height - 1,
        0);
}

void UI::draw()
{
}

void UI::drawHeader()
{
}

void UI::drawSidebar()
{
}

void UI::drawEditor()
{
}

void UI::drawStatusBar()
{
}

int UI::getEditorWidth() const
{
    return 0;
}

int UI::getEditorHeight() const
{
    return 0;
}
