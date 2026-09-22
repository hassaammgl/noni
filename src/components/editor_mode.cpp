#include "editor_impl.hpp"

EditorMode Editor::get_mode() const
{
    return tab ? tab->mode : EditorMode::Normal;
}

std::string Editor::get_mode_label() const
{
    switch (get_mode())
    {
    case EditorMode::Insert:
        return "INSERT";
    case EditorMode::Visual:
        return "VISUAL";
    case EditorMode::VisualLine:
        return "V-LINE";
    default:
        return "NORMAL";
    }
}

void Editor::enter_insert_mode()
{
    if (!tab)
        return;
    pending_j = false;
    clear_pending();
    if (in_visual())
        leave_visual();
    clamp_cursor();
    if (!tab->buffer().is_edit_active())
        tab->buffer().begin_edit(tab->cursor().line, tab->cursor().column);
    tab->mode = EditorMode::Insert;
}

void Editor::leave_insert_mode()
{
    if (!tab)
        return;

    pending_j = false;
    if (tab->mode != EditorMode::Insert)
        return;

    if (tab->cursor().column > 0)
    {
        const auto &lines = tab->buffer().lines();
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
            tab->cursor().column = static_cast<int>(
                TextMetrics::prev_cp(row, static_cast<std::size_t>(tab->cursor().column)));
        }
    }
    tab->mode = EditorMode::Normal;
    tab->buffer().end_edit(tab->cursor().line, tab->cursor().column);
    sync_preferred_display(tab->active_window());
}

void Editor::enter_normal_mode()
{
    if (!tab)
        return;

    clear_pending();
    if (tab->mode == EditorMode::Insert)
        leave_insert_mode();
    else if (in_visual())
        leave_visual();
    else
        tab->mode = EditorMode::Normal;
}

void Editor::begin_buffer_edit()
{
    if (!tab)
        return;
    clamp_cursor();
    tab->buffer().begin_edit(tab->cursor().line, tab->cursor().column);
}

void Editor::end_buffer_edit()
{
    if (!tab)
        return;
    clamp_cursor();
    tab->buffer().end_edit(tab->cursor().line, tab->cursor().column);
}

void Editor::split_insert_edit()
{
    if (!tab || tab->mode != EditorMode::Insert)
        return;
    end_buffer_edit();
    begin_buffer_edit();
}
