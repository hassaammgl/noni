#include "ui_impl.hpp"

void UI::handle_sidebar_input(int ch)
{
    if (ch == 27 || ch == 3)
    {
        return_to_normal();
        return;
    }

    if (dispatch_resolved(ch, InputContext::Sidebar))
        return;

    const SidebarAction action = sidebar.handle_input(ch);
    if (action == SidebarAction::OpenFile)
    {
        const fs::path path = sidebar.get_selected_path();
        if (!path.empty() && fs::is_regular_file(path))
        {
            core.buffers().open_file(path);
            note_opened_file(path);
            sync_active_tab();
            editor.enter_normal_mode();
            refresh_scm(path);
            focus = Focus::Editor;
            keys.clear_chord();
        }
        return;
    }
    if (action == SidebarAction::FocusEditor)
    {
        return_to_normal();
        return;
    }
    if (action == SidebarAction::AddFile)
    {
        open_sidebar_prompt(PromptIntent::AddFile);
        return;
    }
    if (action == SidebarAction::AddFolder)
    {
        open_sidebar_prompt(PromptIntent::AddFolder);
        return;
    }
    if (action == SidebarAction::Rename)
    {
        open_sidebar_prompt(PromptIntent::Rename);
        return;
    }
    if (action == SidebarAction::Delete)
    {
        open_delete_confirm();
        return;
    }
    if (action == SidebarAction::Cut)
    {
        if (sidebar.cut_selected())
            Messages::info(std::format(
                "Cut {}",
                sidebar.get_clipboard_path().filename().string()));
        else
            Messages::error("Nothing to cut");
        return;
    }
    if (action == SidebarAction::Copy)
    {
        if (sidebar.copy_selected())
            Messages::info(std::format(
                "Copied {}",
                sidebar.get_clipboard_path().filename().string()));
        else
            Messages::error("Nothing to copy");
        return;
    }
    if (action == SidebarAction::Paste)
    {
        const auto mode = sidebar.get_clipboard_mode();
        fs::path from;
        fs::path to;
        if (!sidebar.paste_here(&from, &to))
        {
            if (!sidebar.has_clipboard())
                Messages::error("Clipboard empty (use x to cut, c to copy)");
            else
                Messages::error("Could not paste here");
            return;
        }

        Messages::info(std::format(
            "{} {} → {}",
            mode == SidebarClipboardMode::Cut ? "Moved" : "Copied",
            from.filename().string(),
            to.string()));

        if (mode == SidebarClipboardMode::Cut &&
            core.buffers().has_tabs() && !from.empty() && !to.empty() &&
            core.buffers().active().buffer().get_buffer_path() == from)
        {
            core.buffers().active().buffer().set_buffer_path(to);
            sync_active_tab();
        }
        file_picker.reindex();
        return;
    }
}
