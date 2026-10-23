#include "ui_impl.hpp"

void UI::sync_active_tab()
{
    if (!core.buffers().has_tabs())
        core.buffers().open_untitled();

    editor.bind(&core.buffers().active());
    statusbar.set_filename(core.buffers().active().display_name());
    core.attach_lsp_document(core.buffers().active().buffer());

    const fs::path path = core.buffers().active().buffer().get_buffer_path();
    sidebar.set_active_file(path);
    if (!path.empty())
    {
        sidebar.reveal_path(path);
        fs_watcher_.watch(path);
    }
}

bool UI::close_active_tab(bool force)
{
    if (!core.buffers().has_tabs())
        return true;

    if (!force && core.buffers().active().buffer().is_dirty())
    {
        open_save_confirm(ConfirmIntent::CloseTab);
        return false;
    }

    Buffer *closed = &core.buffers().active().buffer();
    core.lsp().notify_close(*closed);
    core.on_buffer_closed(closed);
    const bool was_last = core.buffers().size() == 1;
    core.buffers().close_active(true);

    if (was_last)
    {
        if (running)
            core.buffers().open_untitled();
        sync_active_tab();
        return true;
    }

    sync_active_tab();
    return true;
}

void UI::open_save_confirm(ConfirmIntent intent)
{
    if (command_line.is_active())
        command_line.close();
    if (messages_panel.is_active())
        messages_panel.close();
    if (file_picker.is_active())
        file_picker.close();

    keys.clear_chord();
    editor.enter_normal_mode();

    confirm_intent = intent;
    const std::string name = core.buffers().active().display_name();
    confirm_prompt.open(std::format("Save changes to \"{}\"?", name));
    focus = Focus::Confirm;
    resize();
}

bool UI::save_active_buffer()
{
    Buffer &buf = editor.get_buffer();
    if (buf.get_buffer_path().empty())
    {
        Messages::error("E32: No file name (use :w <path>)");
        return false;
    }
    if (buf.has_load_error())
    {
        Messages::error(std::format(
            "E13: Cannot save — file was not loaded ({})",
            buf.get_load_error()));
        return false;
    }

    if (config.format_on_save && core.lsp().session_supports_formatting(buf))
    {
        const int id = core.lsp().request_formatting(buf);
        if (id > 0)
        {
            const auto deadline =
                std::chrono::steady_clock::now() + std::chrono::seconds(2);
            while (std::chrono::steady_clock::now() < deadline)
            {
                core.lsp().pump();
                if (auto fr = core.lsp().take_format_result())
                {
                    apply_lsp_format_edits(std::move(fr->edits), fr->utf16_pending);
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
    }

    return write_buffer_to_disk();
}

void UI::finish_close_or_quit(ConfirmIntent intent, bool force)
{
    confirm_prompt.close();
    confirm_intent = ConfirmIntent::None;
    focus = Focus::Editor;
    resize();

    if (intent == ConfirmIntent::Quit)
    {
        if (core.buffers().size() > 1)
        {
            close_active_tab(force);
            return;
        }
        if (!force && core.buffers().active().buffer().is_dirty())
            return;
        request_quit();
        return;
    }

    close_active_tab(force);
}

void UI::resolve_save_confirm(ConfirmChoice choice)
{
    if (confirm_intent == ConfirmIntent::DeletePath)
    {
        resolve_delete_confirm(choice);
        return;
    }
    if (confirm_intent == ConfirmIntent::DiscardGit)
    {
        resolve_discard_confirm(choice);
        return;
    }
    if (confirm_intent == ConfirmIntent::RecoverBuffer)
    {
        resolve_recovery_confirm(choice);
        return;
    }

    const ConfirmIntent intent = confirm_intent;

    if (choice == ConfirmChoice::Cancel || choice == ConfirmChoice::Pending)
    {
        if (choice == ConfirmChoice::Cancel)
        {
            confirm_prompt.close();
            confirm_intent = ConfirmIntent::None;
            focus = Focus::Editor;
            editor.enter_normal_mode();
            resize();
            Messages::info("Save cancelled");
        }
        return;
    }

    if (choice == ConfirmChoice::Yes)
    {
        if (!save_active_buffer())
            return;
        finish_close_or_quit(intent, true);
        return;
    }

    finish_close_or_quit(intent, true);
}
