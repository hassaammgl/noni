#include <components/editor.hpp>

void Editor::moveCursorUp()
{
}

void Editor::moveCursorDown()
{
}
void Editor::moveCursorLeft()
{
}
void Editor::moveCursorRight()
{
}
void Editor::updateScroll()
{
}
void Editor::draw()
{
    if (!window)
        return;

    werase(window);
    wbkgd(window, COLOR_PAIR(1));
    mvwprintw(
        window,
        0,
        0,
        "NONI EDITOR");
    wmove(window, cursor.line, cursor.column);
    wnoutrefresh(window);
}

void Editor::handleInput(int key)
{
    switch (key)
    {
    case KEY_UP:
        this->moveCursorUp();
        break;
    case KEY_DOWN:
        this->moveCursorDown();
        break;
    case KEY_LEFT:
        this->moveCursorLeft();
        break;
    case KEY_RIGHT:
        this->moveCursorRight();
        break;

    default:
        break;
    }
    this->updateScroll();
}

void Editor::setCursorPosition(int line, int column)
{
    this->cursor = {.line = line, .column = column};
}