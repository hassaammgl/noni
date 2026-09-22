#include "ui_impl.hpp"

void UI::render()
{
    if (!core.buffers().has_tabs())
        return;

    sync_scm_ui();
    sync_scm_diff();
    tick_recovery_snapshots();
    core.lsp().pump();
    poll_completion_result();
    poll_lsp_results();

    Cursor c = editor.get_cursor();
    int chr = 1;
    int dcol = 1;
    const auto &lines = editor.get_buffer().lines();
    if (!lines.empty() && c.line >= 0 && c.line < static_cast<int>(lines.size()))
    {
        const auto &row = lines[static_cast<std::size_t>(c.line)];
        chr = TextMetrics::byte_to_codepoint_index(row, static_cast<std::size_t>(c.column)) + 1;
        dcol = TextMetrics::byte_to_display(row, static_cast<std::size_t>(c.column)) + 1;
    }
    statusbar.set_cursor_position(c.line + 1, chr, dcol);
    update_statusbar_mode();
    update_cursor_visibility();
    sync_messages_echo();

    const ScmFileDiff *diff_ptr = nullptr;
    if (Buffer *b = core.active_buffer(); b && !b->get_buffer_path().empty())
    {
        const fs::path path = b->get_buffer_path();
        const std::uint64_t gen = core.scm().diff_generation();
        if (!scm_diff_cache_ || path != scm_diff_cache_path_ || gen != scm_diff_cache_gen_)
        {
            scm_diff_cache_ = core.scm().file_diff(path);
            scm_diff_cache_path_ = path;
            scm_diff_cache_gen_ = gen;
        }
        if (scm_diff_cache_)
            diff_ptr = &*scm_diff_cache_;
    }
    else
    {
        scm_diff_cache_.reset();
        scm_diff_cache_path_.clear();
        scm_diff_cache_gen_ = 0;
    }

    line_number.sync(
        editor.get_scroll_y(),
        c.line,
        static_cast<int>(editor.get_buffer().lines().size()),
        &editor.get_buffer().diagnostics(),
        diff_ptr);

    // Do NOT werase(stdscr) every frame — that blanks the whole terminal and
    // causes visible flicker on mode / focus changes. Panels paint their own cells.
    wbkgd(stdscr, COLOR_PAIR(Theme::Editor));

    header.draw();
    tab_bar.draw();
    if (sidebar_visible)
    {
        if (side_view == SideView::Search)
        {
            search_panel.set_focused(focus == Focus::Search);
            search_panel.draw();
        }
        else
        {
            sidebar.set_focused(focus == Focus::Sidebar);
            sidebar.draw();
        }
    }
    line_number.draw();
    editor.draw();

    if (messages_panel.is_active())
        messages_panel.draw();

    if (file_picker.is_active())
        file_picker.draw();
    if (buffer_picker.is_active())
        buffer_picker.draw();
    if (lsp_picker.is_active())
        lsp_picker.draw();
    if (completion_picker.is_active())
        completion_picker.draw();

    statusbar.draw();

    if (terminal_visible)
    {
        terminal.set_focused(focus == Focus::Terminal);
        terminal.draw();
    }

    if (command_line.is_active())
        command_line.draw();
    else if (confirm_prompt.is_active())
        confirm_prompt.draw();
    else if (input_prompt.is_active())
        input_prompt.draw();

    wnoutrefresh(header.get_window());
    wnoutrefresh(tab_bar.get_window());
    if (sidebar_visible)
    {
        if (side_view == SideView::Search)
            wnoutrefresh(search_panel.get_window());
        else
            wnoutrefresh(sidebar.get_window());
    }
    wnoutrefresh(line_number.get_window());

    if (terminal_visible)
        wnoutrefresh(terminal.get_window());

    wnoutrefresh(statusbar.get_window());

    if (command_line.is_active())
    {
        wnoutrefresh(editor.get_window());
        wnoutrefresh(command_line.get_window());
    }
    else if (confirm_prompt.is_active())
    {
        wnoutrefresh(editor.get_window());
        wnoutrefresh(confirm_prompt.get_window());
    }
    else if (input_prompt.is_active())
    {
        wnoutrefresh(editor.get_window());
        wnoutrefresh(input_prompt.get_window());
    }
    else if (file_picker.is_active())
    {
        wnoutrefresh(editor.get_window());
        wnoutrefresh(file_picker.get_window());
    }
    else if (buffer_picker.is_active())
    {
        wnoutrefresh(editor.get_window());
        wnoutrefresh(buffer_picker.get_window());
    }
    else if (lsp_picker.is_active())
    {
        wnoutrefresh(editor.get_window());
        wnoutrefresh(lsp_picker.get_window());
    }
    else if (completion_picker.is_active())
    {
        wnoutrefresh(editor.get_window());
        wnoutrefresh(completion_picker.get_window());
    }
    else if (messages_panel.is_active())
    {
        // Editor underneath, help/messages on top (must not be covered).
        wnoutrefresh(editor.get_window());
        wnoutrefresh(messages_panel.get_window());
    }
    else if (focus == Focus::Terminal && terminal_visible)
    {
        wnoutrefresh(editor.get_window());
        wnoutrefresh(terminal.get_window());
    }
    else
    {
        wnoutrefresh(editor.get_window());
    }

    doupdate();
}
