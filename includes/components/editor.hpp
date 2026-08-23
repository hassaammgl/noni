#pragma once

#include <ui/UIComponent.hpp>
#include <utils/cursor.hpp>
#include <editor/buffer.hpp>

class Editor : public UIComponent
{
private:
    int cursorLine = 0;
    int cursorColumn = 0;
    Cursor cursor = {.line = 1, .column = 1};

    void moveCursorUp();
    void moveCursorDown();
    void moveCursorLeft();
    void moveCursorRight();
    void updateScroll();

public:
    Buffer buffer;
    void draw() override;

    void handleInput(int key);

    void setCursorPosition(int line, int column);
};