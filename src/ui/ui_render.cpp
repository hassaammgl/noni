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
    tick_autosave();

    Cursor c = editor.get_cursor();
    int chr = 1;
    int dcol = 1;
    const auto &lines = editor.get_buffer().lines();
    if (!lines.empty() && c.line >= 0 && c.line < static_cast<int>(lines.size()))
    {
        const auto &row = lines[static_cast<std::size_t>(c.line)];
        chr = TextMetrics::byte_to_codepoint_index(row, static_cast<std::size_t>(c.column)) + 1;
        dcol = editor.get_buffer().byte_to_display_cached(
                  c.line, static_cast<std::size_t>(c.column)) +
              1;
    }
    statusbar.set_cursor_position(c.line + 1, chr, dcol);
    statusbar.set_chord(keys.has_pending() ? keys.pending_label() : "");
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

    wbkgd(stdscr, COLOR_PAIR(Theme::Editor));
    mark_components_from_state();
    render_paint();
}
