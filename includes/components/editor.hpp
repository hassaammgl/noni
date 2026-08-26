#pragma once

#include <ui/UIComponent.hpp>
#include <utils/cursor.hpp>
#include <editor/buffer.hpp>

class Editor : public UIComponent
{
private:
    Cursor cursor = {.line = 0, .column = 0};
    int scroll_y = 0;
    void move_cursor_up();
    void move_cursor_down();
    void move_cursor_left();
    void move_cursor_right();
    void update_scroll();
    void clamp_cursor();

public:
    Cursor get_cursor() const;
    Buffer buffer;
    void draw() override;

    void handle_input(int key);

    void set_cursor_position(int line, int column);
};
