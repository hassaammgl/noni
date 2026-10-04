#include "ui_impl.hpp"

void UI::render_paint()
{
    bool any = false;

    if (sidebar_visible)
    {
        if (side_view == SideView::Search)
        {
            search_panel.set_focused(focus == Focus::Search);
            any |= paint_and_refresh(search_panel);
        }
        else
        {
            sidebar.set_focused(focus == Focus::Sidebar);
            any |= paint_and_refresh(sidebar);
        }
    }

    any |= paint_and_refresh(header);
    any |= paint_and_refresh(tab_bar);
    any |= paint_and_refresh(line_number);
    any |= paint_and_refresh(editor);

    if (messages_panel.is_active())
        any |= paint_and_refresh(messages_panel);
    if (file_picker.is_active())
        any |= paint_and_refresh(file_picker);
    if (buffer_picker.is_active())
        any |= paint_and_refresh(buffer_picker);
    if (lsp_picker.is_active())
        any |= paint_and_refresh(lsp_picker);
    if (completion_picker.is_active())
        any |= paint_and_refresh(completion_picker);
    if (hover_popup_.is_active())
        any |= paint_and_refresh(hover_popup_);

    any |= paint_and_refresh(statusbar);

    if (terminal_visible)
    {
        terminal.set_focused(focus == Focus::Terminal);
        any |= paint_and_refresh(terminal);
    }

    if (command_line.is_active())
        any |= paint_and_refresh(command_line);
    else if (confirm_prompt.is_active())
        any |= paint_and_refresh(confirm_prompt);
    else if (input_prompt.is_active())
        any |= paint_and_refresh(input_prompt);

    // Overlay z-order: re-stack top window when active (even if already painted).
    auto restack = [&](UIComponent &under, UIComponent &over) {
        if (WINDOW *w = under.get_window())
            wnoutrefresh(w);
        if (WINDOW *w = over.get_window())
            wnoutrefresh(w);
        any = true;
    };

    if (command_line.is_active())
        restack(editor, command_line);
    else if (confirm_prompt.is_active())
        restack(editor, confirm_prompt);
    else if (input_prompt.is_active())
        restack(editor, input_prompt);
    else if (file_picker.is_active())
        restack(editor, file_picker);
    else if (buffer_picker.is_active())
        restack(editor, buffer_picker);
    else if (lsp_picker.is_active())
        restack(editor, lsp_picker);
    else if (completion_picker.is_active())
        restack(editor, completion_picker);
    else if (hover_popup_.is_active())
        restack(editor, hover_popup_);
    else if (messages_panel.is_active())
        restack(editor, messages_panel);
    else if (focus == Focus::Terminal && terminal_visible)
        restack(editor, terminal);

    if (any)
        doupdate();

    // Snapshot painted state for next delta pass.
    painted_buf_rev_ = editor.get_buffer().get_revision();
    painted_cursor_ = editor.get_cursor();
    painted_scroll_y_ = editor.get_scroll_y();
    painted_scroll_x_ = editor.get_scroll_x();
    painted_mode_ = editor.get_mode();
    painted_focus_ = focus;
    painted_echo_seq_ = Messages::echo_snapshot().first;
    painted_diag_gen_ = editor.get_buffer().diagnostics().generation;
    painted_scm_gen_ = core.scm().generation();
    painted_sidebar_visible_ = sidebar_visible;
    painted_side_view_ = side_view;
    painted_terminal_visible_ = terminal_visible;
    force_paint_all_ = false;
}
