#include "editor_impl.hpp"

bool Editor::apply_completion(const CompletionItem &item)
{
    if (!tab || !tab->active_window().has_buffer())
        return false;

    std::string text = item.new_text;
    if (text.empty())
        text = !item.insert_text.empty() ? item.insert_text : item.label;
    if (text.empty())
        return false;

    Cursor start = tab->cursor();
    Cursor end = start;
    if (item.has_text_edit)
    {
        start = item.edit_start;
        end = item.edit_end;
        if (end.line < start.line || (end.line == start.line && end.column < start.column))
            std::swap(start, end);
    }

    const bool nested = tab->buffer().is_edit_active();
    if (!nested)
        begin_buffer_edit();

    if (start.line != end.line || start.column != end.column)
        tab->buffer().delete_range(start.line, start.column, end.line, end.column);

    const auto [el, ec] = tab->buffer().insert_text(start.line, start.column, text);
    tab->cursor().line = el;
    tab->cursor().column = ec;
    leave_visual();
    clamp_cursor();
    sync_preferred_display(tab->active_window());
    update_scroll();

    if (!nested)
        end_buffer_edit();
    return true;
}

bool Editor::paste_clipboard()
{
    if (!tab)
        return false;

    const auto text = Clipboard::read();
    if (!text || text->empty())
        return false;

    RegisterValue value;
    value.text = *text;
    value.type = RegisterType::Character;
    registers().set_unnamed(std::move(value));
    return paste_register(true);
}

bool Editor::undo()
{
    if (!tab)
        return false;

    if (tab->mode == EditorMode::Insert)
        leave_insert_mode();
    else if (in_visual())
        leave_visual();
    else if (tab->buffer().is_edit_active())
        end_buffer_edit();

    clear_pending();

    const UndoResult result = tab->buffer().undo();
    if (!result.ok)
        return false;

    tab->cursor().line = result.cursor_line;
    tab->cursor().column = result.cursor_col;
    clamp_cursor();
    sync_preferred_display(tab->active_window());
    update_scroll();
    return true;
}

bool Editor::redo()
{
    if (!tab)
        return false;

    if (tab->mode == EditorMode::Insert)
        leave_insert_mode();
    else if (in_visual())
        leave_visual();
    else if (tab->buffer().is_edit_active())
        end_buffer_edit();

    clear_pending();

    const UndoResult result = tab->buffer().redo();
    if (!result.ok)
        return false;

    tab->cursor().line = result.cursor_line;
    tab->cursor().column = result.cursor_col;
    clamp_cursor();
    sync_preferred_display(tab->active_window());
    update_scroll();
    return true;
}
