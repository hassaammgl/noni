#include "editor_impl.hpp"

void Editor::bind_core(EditorCore *core)
{
    core_ = core;
}

void Editor::bind(EditorTab *new_tab)
{
    tab = new_tab;
    pending_j = false;
    clear_pending();
}

void Editor::set_tab_switch_handlers(std::function<void()> next, std::function<void()> prev)
{
    on_next_tab_ = std::move(next);
    on_prev_tab_ = std::move(prev);
}

void Editor::set_search_handlers(std::function<void()> forward, std::function<void()> backward)
{
    on_search_forward_ = std::move(forward);
    on_search_backward_ = std::move(backward);
}

Window &Editor::win()
{
    return tab->active_window();
}

const Window &Editor::win() const
{
    return tab->active_window();
}

RegisterFile &Editor::registers()
{
    return core_->registers();
}

MarkTable &Editor::marks()
{
    return core_->marks();
}

JumpList &Editor::jumps()
{
    return core_->jumps();
}

void Editor::handle_input(int key)
{
    if (!tab)
        return;

    if (tab->mode == EditorMode::Normal)
        handle_normal_input(key);
    else if (in_visual())
        handle_visual_input(key);
    else
        handle_insert_input(key);
}

void Editor::set_cursor_position(int line, int column)
{
    if (!tab)
        return;

    record_jump_from_here();
    tab->cursor() = {.line = line, .column = column};
    clamp_cursor();
    sync_preferred_display(tab->active_window());
    tab->scroll_y() = 0;
    tab->scroll_x() = 0;
}

Cursor Editor::get_cursor() const
{
    return tab ? tab->cursor() : Cursor{.line = 0, .column = 0};
}

int Editor::get_scroll_y() const
{
    return tab ? tab->scroll_y() : 0;
}
