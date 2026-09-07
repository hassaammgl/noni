#pragma once

#include <ui/UIComponent.hpp>
#include <utils/cursor.hpp>
#include <editor/buffer.hpp>
#include <string>

enum class EditorMode
{
    Normal,
    Insert,
};

class Editor : public UIComponent
{
private:
    Cursor cursor = {.line = 0, .column = 0};
    int scroll_y = 0;
    EditorMode mode = EditorMode::Normal;

    void move_cursor_up();
    void move_cursor_down();
    void move_cursor_left();
    void move_cursor_right();
    void update_scroll();
    void clamp_cursor();
    void handle_normal_input(int key);
    void handle_insert_input(int key);

public:
    Cursor get_cursor() const;
    EditorMode get_mode() const;
    std::string get_mode_label() const;

    void set_mode(EditorMode mode);
    void enter_insert_mode();
    void enter_normal_mode();

    Buffer buffer;
    void draw() override;

    void handle_input(int key);

    void set_cursor_position(int line, int column);
    int get_scroll_y() const;
};
