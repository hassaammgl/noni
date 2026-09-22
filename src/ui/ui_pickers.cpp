#include "ui_impl.hpp"

void UI::sync_scm_ui()
{
    const std::uint64_t gen = core.scm().generation();
    if (gen == scm_gen_seen_)
    {
        // Still refresh per-buffer badge cheaply from cache (no Git).
        if (Buffer *b = core.active_buffer())
        {
            if (auto st = core.scm().status_for(b->get_buffer_path()))
                statusbar.set_scm_badge(std::format("{}{}", st->xy[0], st->xy[1]));
            else if (core.scm().snapshot().is_repo && !b->get_buffer_path().empty())
                statusbar.set_scm_badge("");
        }
        return;
    }
    scm_gen_seen_ = gen;

    const auto snap = core.scm().snapshot();
    std::string label = snap.branch;
    if (snap.is_repo && (snap.ahead > 0 || snap.behind > 0))
    {
        if (snap.ahead > 0)
            label += std::format(" ↑{}", snap.ahead);
        if (snap.behind > 0)
            label += std::format(" ↓{}", snap.behind);
    }
    header.set_branch(std::move(label));

    if (Buffer *b = core.active_buffer())
    {
        if (auto st = core.scm().status_for(b->get_buffer_path()))
            statusbar.set_scm_badge(std::format("{}{}", st->xy[0], st->xy[1]));
        else
            statusbar.set_scm_badge("");
    }
    else
    {
        statusbar.set_scm_badge("");
    }
}

void UI::open_file_search()
{
    keys.clear_chord();
    if (command_line.is_active())
        command_line.close();
    if (messages_panel.is_active())
        messages_panel.close();
    if (buffer_picker.is_active())
        buffer_picker.close();
    if (confirm_prompt.is_active())
    {
        confirm_prompt.close();
        confirm_intent = ConfirmIntent::None;
    }

    editor.enter_normal_mode();
    file_picker.set_recent(core.recent().list());
    file_picker.open(project_root());
    focus = Focus::FileSearch;
    resize();
}

void UI::close_file_search(bool open_selected)
{
    fs::path path;
    const bool has_sel = open_selected && file_picker.take_selection(path);
    file_picker.close();
    focus = Focus::Editor;
    keys.clear_chord();
    resize();

    if (!has_sel)
        return;

    core.buffers().open_file(path);
    note_opened_file(path);
    sync_active_tab();
    editor.enter_normal_mode();
    refresh_scm(path);
    Messages::info(std::format("\"{}\"", core.buffers().active().display_name()));
}

void UI::open_buffer_search()
{
    keys.clear_chord();
    if (command_line.is_active())
        command_line.close();
    if (messages_panel.is_active())
        messages_panel.close();
    if (file_picker.is_active())
        file_picker.close();
    if (completion_picker.is_active())
        completion_picker.close();

    editor.enter_normal_mode();
    buffer_picker.bind(&core.buffers());
    buffer_picker.open();
    focus = Focus::BufferSearch;
    resize();
}

void UI::close_buffer_search(bool open_selected)
{
    int idx = -1;
    const bool has_sel = open_selected && buffer_picker.take_selection(idx);
    buffer_picker.close();
    focus = Focus::Editor;
    keys.clear_chord();
    resize();

    if (!has_sel || idx < 0)
        return;
    if (idx >= static_cast<int>(core.buffers().size()))
        return;
    core.buffers().switch_to(idx);
    sync_active_tab();
    editor.enter_normal_mode();
}

void UI::open_workspace_prompt()
{
    if (command_line.is_active())
        command_line.close();
    if (confirm_prompt.is_active())
        confirm_prompt.close();
    if (file_picker.is_active())
        file_picker.close();
    if (buffer_picker.is_active())
        buffer_picker.close();

    keys.clear_chord();
    prompt_intent = PromptIntent::OpenWorkspace;
    input_prompt.open("Open workspace: ", project_root().string());
    focus = Focus::Prompt;
    resize();
}
