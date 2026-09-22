#include "editor_impl.hpp"

void Editor::clear_pending()
{
    pending_g = false;
    pending_m = false;
    pending_jump_mark = false;
    pending_register = false;
    pending_op = PendingOperator::None;
    if (core_)
        registers().clear_pending();
}

bool Editor::in_visual() const
{
    return tab &&
           (tab->mode == EditorMode::Visual || tab->mode == EditorMode::VisualLine);
}

void Editor::leave_visual()
{
    if (!tab)
        return;
    tab->active_window().clear_selection();
    tab->mode = EditorMode::Normal;
}

void Editor::enter_visual_char()
{
    if (!tab)
        return;
    clear_pending();
    clamp_cursor();
    tab->mode = EditorMode::Visual;
    tab->active_window().enter_visual(SelectionKind::Character);
}

void Editor::enter_visual_line()
{
    if (!tab)
        return;
    clear_pending();
    clamp_cursor();
    tab->mode = EditorMode::VisualLine;
    tab->active_window().enter_visual(SelectionKind::Line);
}
