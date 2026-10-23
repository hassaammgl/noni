#include "editor_impl.hpp"

void Editor::cmd_select_all()
{
    if (!tab || !tab->active_window().has_buffer())
        return;

    clear_pending();
    if (tab->mode == EditorMode::Insert)
        leave_insert_mode();

    tab->cursor() = {.line = 0, .column = 0};
    enter_visual_line();
    apply_motion(Motion::file_end(win()), true);
    clamp_cursor();
    update_scroll();
}

void Editor::cmd_delete_line()
{
    if (!tab || !tab->active_window().has_buffer())
        return;

    const bool was_insert = tab->mode == EditorMode::Insert;
    if (was_insert)
        leave_insert_mode();
    else if (in_visual())
        leave_visual();

    clear_pending();
    clamp_cursor();
    TextRange range = normalize_line_range(
        tab->cursor(), tab->cursor(), static_cast<int>(tab->buffer().lines().size()));
    execute_operator(PendingOperator::Delete, range, false);
    clamp_cursor();
    update_scroll();
    if (was_insert)
        enter_insert_mode();
}
