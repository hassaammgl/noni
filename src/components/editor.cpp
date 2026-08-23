#include <components/editor.hpp>

void Editor::moveCursorUp()
{
}
void Editor::moveCursorDown()
{
}
void Editor::moveCursorLeft()
{
    wmove(window, cursor.line, cursor.column - 1);
}
void Editor::moveCursorRight()
{
    wmove(window, cursor.line, cursor.column + 1);
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
    auto content = buffer.readBuffer();
    int line = 0;
    for (auto c : content)
    {
        mvwprintw(
            window,
            0 + line,
            0,
            c.c_str());
        line++;
    }
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