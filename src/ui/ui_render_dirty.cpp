#include "ui_impl.hpp"

void UI::mark_all_components_dirty()
{
    force_paint_all_ = true;
    header.mark_dirty();
    tab_bar.mark_dirty();
    sidebar.mark_dirty();
    search_panel.mark_dirty();
    line_number.mark_dirty();
    editor.mark_dirty();
    messages_panel.mark_dirty();
    file_picker.mark_dirty();
    buffer_picker.mark_dirty();
    lsp_picker.mark_dirty();
    completion_picker.mark_dirty();
    hover_popup_.mark_dirty();
    statusbar.mark_dirty();
    command_line.mark_dirty();
    confirm_prompt.mark_dirty();
    input_prompt.mark_dirty();
    terminal.mark_dirty();
}

void UI::mark_components_from_state()
{
    if (force_paint_all_)
    {
        mark_all_components_dirty();
        return;
    }

    const Buffer &buf = editor.get_buffer();
    const Cursor c = editor.get_cursor();
    const int sy = editor.get_scroll_y();
    const int sx = editor.get_scroll_x();
    const EditorMode mode = editor.get_mode();
    const auto [echo_seq, echo_text] = Messages::echo_snapshot();
    (void)echo_text;

    bool editor_dirty = false;
    if (buf.get_revision() != painted_buf_rev_ ||
        c.line != painted_cursor_.line || c.column != painted_cursor_.column ||
        sy != painted_scroll_y_ || sx != painted_scroll_x_ ||
        mode != painted_mode_ ||
        buf.diagnostics().generation != painted_diag_gen_)
        editor_dirty = true;

    if (editor_dirty)
    {
        editor.mark_dirty();
        line_number.mark_dirty();
    }
    if (editor_dirty || mode != painted_mode_ || focus != painted_focus_ ||
        echo_seq != painted_echo_seq_ || keys.has_pending() ||
        core.scm().generation() != painted_scm_gen_)
        statusbar.mark_dirty();

    if (focus != painted_focus_ || sidebar_visible != painted_sidebar_visible_ ||
        side_view != painted_side_view_)
    {
        sidebar.mark_dirty();
        search_panel.mark_dirty();
        editor.mark_dirty();
        line_number.mark_dirty();
        statusbar.mark_dirty();
    }
    if (terminal_visible != painted_terminal_visible_ || focus == Focus::Terminal)
        terminal.mark_dirty();

    if (buf.get_revision() != painted_buf_rev_ || force_paint_all_)
        tab_bar.mark_dirty();

    if (file_picker.is_active())
        file_picker.mark_dirty();
    if (buffer_picker.is_active())
        buffer_picker.mark_dirty();
    if (lsp_picker.is_active())
        lsp_picker.mark_dirty();
    if (completion_picker.is_active())
        completion_picker.mark_dirty();
    if (hover_popup_.is_active())
        hover_popup_.mark_dirty();
    if (messages_panel.is_active())
        messages_panel.mark_dirty();
    if (command_line.is_active())
        command_line.mark_dirty();
    if (confirm_prompt.is_active())
        confirm_prompt.mark_dirty();
    if (input_prompt.is_active())
        input_prompt.mark_dirty();
}

bool UI::paint_and_refresh(UIComponent &c)
{
    if (!c.paint_if_dirty())
        return false;
    if (WINDOW *w = c.get_window())
        wnoutrefresh(w);
    return true;
}
