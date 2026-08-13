#pragma once

#include "ui/UIComponent.hpp"
#include <utils/cursor.hpp>

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
    void draw() override;

    void handleInput(int key);

    void setCursorPosition(int line, int column);
};