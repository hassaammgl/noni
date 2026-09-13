#pragma once

#include <editor/editor_tab.hpp>
#include <ui/UIComponent.hpp>
#include <string>

class Editor : public UIComponent
{
private:
    EditorTab *tab = nullptr;
    bool pending_j = false;

    void move_cursor_up();
    void move_cursor_down();
    void move_cursor_left();
    void move_cursor_right();
    void page_up();
    void page_down();
    void half_page_up();
    void half_page_down();
    void update_scroll();
    void clamp_cursor();
    void handle_normal_input(int key);
    void handle_insert_input(int key);
    void leave_insert_mode();
    int page_step() const;

public:
    void bind(EditorTab *tab);

    Cursor get_cursor() const;
    EditorMode get_mode() const;
    std::string get_mode_label() const;

    void enter_insert_mode();
    void enter_normal_mode();

    Buffer &get_buffer();
    const Buffer &get_buffer() const;

    void draw() override;
    void handle_input(int key);

    void set_cursor_position(int line, int column);
    int get_scroll_y() const;
};
