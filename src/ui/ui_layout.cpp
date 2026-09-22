#include "ui_impl.hpp"

void UI::update_statusbar_mode()
{
    switch (focus)
    {
    case Focus::Command:
        statusbar.set_mode("COMMAND");
        break;
    case Focus::Messages:
        statusbar.set_mode("MESSAGES");
        break;
    case Focus::Sidebar:
        statusbar.set_mode("SIDEBAR");
        break;
    case Focus::Search:
        statusbar.set_mode("SEARCH");
        break;
    case Focus::FileSearch:
        statusbar.set_mode("FILES");
        break;
    case Focus::BufferSearch:
        statusbar.set_mode("BUFFERS");
        break;
    case Focus::LspPicker:
        statusbar.set_mode("LSP");
        break;
    case Focus::Completion:
        statusbar.set_mode("COMPLETE");
        break;
    case Focus::Confirm:
        statusbar.set_mode("CONFIRM");
        break;
    case Focus::Prompt:
        statusbar.set_mode("INPUT");
        break;
    case Focus::Terminal:
        statusbar.set_mode("TERMINAL");
        break;
    case Focus::Editor:
        statusbar.set_mode(editor.get_mode_label());
        break;
    }
}

void UI::update_cursor_visibility()
{
    if (focus == Focus::Command || focus == Focus::FileSearch ||
        focus == Focus::BufferSearch || focus == Focus::LspPicker ||
        focus == Focus::Completion || focus == Focus::Confirm ||
        focus == Focus::Prompt || focus == Focus::Terminal || focus == Focus::Search)
        curs_set(1);
    else if (focus == Focus::Editor)
        curs_set(editor.get_mode() == EditorMode::Insert ? 2 : 1);
    else
        curs_set(0);
}

void UI::resize()
{
    height = get_editor_dim().height;
    width = get_editor_dim().width;

    wbkgd(stdscr, COLOR_PAIR(Theme::Editor));
    erase();
    refresh();

    const int sb = effective_sidebar_width();

    if (height < 5 || width <= sb + line_number_width)
    {
        mvprintw(1, 0, "INVALID DIMENSIONS");
        refresh();
        return;
    }

    const bool cmd_open = command_line.is_active();
    const bool confirm_open = confirm_prompt.is_active();
    const bool prompt_open = input_prompt.is_active();
    const int bottom_rows = (cmd_open || confirm_open || prompt_open) ? 2 : 1;
    const int top_y = 1;

    int term_h = 0;
    if (terminal_visible)
    {
        term_h = std::clamp(terminal_height, 5, std::max(5, (height - bottom_rows - 3) / 2));
    }

    const int pane_height = height - top_y - bottom_rows - term_h;
    const int editor_y = top_y + 1;
    const int editor_height = pane_height - 1;
    const int editor_width = width - sb - line_number_width;
    const int editor_x = sb + line_number_width;
    const int editor_pane_width = width - sb;

    if (pane_height < 2 || editor_height < 1)
        return;

    header.resize(1, width, 0, 0);

    if (sidebar_visible)
    {
        if (side_view == SideView::Search)
        {
            search_panel.resize(pane_height, sb, top_y, 0);
            sidebar.resize(0, 0, 0, 0);
        }
        else
        {
            sidebar.resize(pane_height, sb, top_y, 0);
            search_panel.resize(0, 0, 0, 0);
        }
    }
    else
    {
        sidebar.resize(0, 0, 0, 0);
        search_panel.resize(0, 0, 0, 0);
    }

    tab_bar.resize(1, editor_pane_width, top_y, sb);
    line_number.resize(editor_height, line_number_width, editor_y, sb);
    editor.resize(editor_height, editor_width, editor_y, editor_x);
    messages_panel.resize(editor_height, editor_pane_width, editor_y, sb);

    const int picker_h = std::min(std::max(10, pane_height - 2), 22);
    const int picker_w = std::min(std::max(40, editor_pane_width - 4), 80);
    const int picker_y = top_y + std::max(1, (pane_height - picker_h) / 2);
    const int picker_x = sb + std::max(1, (editor_pane_width - picker_w) / 2);
    file_picker.resize(picker_h, picker_w, picker_y, picker_x);
    buffer_picker.resize(picker_h, picker_w, picker_y, picker_x);
    lsp_picker.resize(picker_h, picker_w, picker_y, picker_x);

    // Completion popup near the active cursor inside the editor pane.
    {
        const Cursor c = editor.get_cursor();
        const int sy = editor.get_scroll_y();
        const int items = completion_picker.is_active()
                              ? static_cast<int>(completion_picker.list().items.size())
                              : 8;
        const int comp_h = std::clamp(items + 2, 4, std::min(14, std::max(4, editor_height)));
        const int comp_w = std::min(48, std::max(24, editor_width - 2));
        int comp_y = editor_y + std::clamp(c.line - sy + 1, 0, std::max(0, editor_height - comp_h));
        int comp_x = editor_x + 2;
        if (comp_x + comp_w > sb + editor_pane_width)
            comp_x = std::max(sb, sb + editor_pane_width - comp_w);
        completion_picker.resize(comp_h, comp_w, comp_y, comp_x);
    }

    const int status_y = height - bottom_rows;
    if (cmd_open || confirm_open || prompt_open)
    {
        statusbar.resize(1, width, height - 2, 0);
        command_line.resize(1, width, height - 1, 0);
        confirm_prompt.resize(1, width, height - 1, 0);
        input_prompt.resize(1, width, height - 1, 0);
    }
    else
    {
        statusbar.resize(1, width, status_y, 0);
        command_line.resize(1, width, status_y, 0);
        confirm_prompt.resize(1, width, status_y, 0);
        input_prompt.resize(1, width, status_y, 0);
    }

    if (terminal_visible && term_h > 0)
    {
        terminal.resize(term_h, width, status_y - term_h, 0);
        terminal.set_visible(true);
        terminal.on_resized();
    }
    else
    {
        terminal.resize(0, 0, 0, 0);
        terminal.set_visible(false);
    }

    refresh();
}
