#include "ui_impl.hpp"

void UI::queue_recovery_prompts()
{
    pending_recovery_ = RecoveryStore::list(core.workspace().root());
    if (!pending_recovery_.empty())
        open_recovery_confirm();
}

void UI::open_recovery_confirm()
{
    if (pending_recovery_.empty())
        return;
    if (command_line.is_active())
        command_line.close();
    if (input_prompt.is_active())
        input_prompt.close();

    const auto &e = pending_recovery_.front();
    confirm_intent = ConfirmIntent::RecoverBuffer;
    confirm_prompt.open(std::format(
        "Recover unsaved changes for {}? [y/n/esc]",
        e.original_path.filename().string()));
    focus = Focus::Confirm;
    resize();
}

void UI::resolve_recovery_confirm(ConfirmChoice choice)
{
    confirm_prompt.close();
    confirm_intent = ConfirmIntent::None;
    focus = Focus::Editor;
    resize();

    if (pending_recovery_.empty())
        return;

    RecoveryEntry entry = pending_recovery_.front();
    pending_recovery_.erase(pending_recovery_.begin());

    if (choice == ConfirmChoice::Yes)
    {
        std::string err;
        auto lines = RecoveryStore::read_lines(entry, &err);
        if (!lines)
        {
            Messages::error(err.empty() ? "Recovery read failed" : err);
        }
        else
        {
            core.buffers().open_file(entry.original_path);
            note_opened_file(entry.original_path);
            Buffer &b = core.buffers().active().buffer();
            b.apply_recovered_content(std::move(*lines));
            core.attach_lsp_document(b);
            sync_active_tab();
            Messages::info(std::format(
                "Recovered {} — save to keep",
                entry.original_path.filename().string()));
        }
        // Keep snapshot until user saves successfully.
    }
    else
    {
        RecoveryStore::clear_snapshot(core.workspace().root(), entry.original_path);
        Messages::info(std::format(
            "Discarded recovery for {}",
            entry.original_path.filename().string()));
    }

    if (!pending_recovery_.empty())
        open_recovery_confirm();
}

void UI::tick_recovery_snapshots()
{
    const auto now = std::chrono::steady_clock::now();
    if (now - last_recovery_tick_ < std::chrono::seconds(2))
        return;
    last_recovery_tick_ = now;

    const fs::path root = core.workspace().root();
    if (root.empty())
        return;

    for (const auto &tab : core.buffers().get_tabs())
    {
        const Buffer &b = tab.buffer();
        const fs::path path = b.get_buffer_path();
        if (path.empty())
            continue;
        if (!b.is_dirty())
        {
            RecoveryStore::clear_snapshot(root, path);
            continue;
        }
        (void)RecoveryStore::write_snapshot(root, path, b.lines(), nullptr);
    }
}

void UI::clear_recovery_for_buffer(const Buffer &buffer)
{
    const fs::path path = buffer.get_buffer_path();
    if (path.empty())
        return;
    RecoveryStore::clear_snapshot(core.workspace().root(), path);
}
