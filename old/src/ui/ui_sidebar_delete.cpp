#include "ui_impl.hpp"

void UI::open_delete_confirm()
{
    const fs::path path = sidebar.get_selected_path();
    if (path.empty() || path == sidebar.get_project_path())
    {
        Messages::error("Cannot delete project root");
        return;
    }

    if (command_line.is_active())
        command_line.close();
    if (input_prompt.is_active())
        input_prompt.close();

    keys.clear_chord();
    pending_delete_path = path;
    confirm_intent = ConfirmIntent::DeletePath;
    confirm_prompt.open(std::format("Delete \"{}\"?", path.filename().string()));
    focus = Focus::Confirm;
    resize();
}

void UI::resolve_delete_confirm(ConfirmChoice choice)
{
    if (choice == ConfirmChoice::Pending)
        return;

    const fs::path path = pending_delete_path;
    confirm_prompt.close();
    confirm_intent = ConfirmIntent::None;
    pending_delete_path.clear();
    focus = Focus::Sidebar;
    resize();

    if (choice != ConfirmChoice::Yes)
    {
        Messages::info("Delete cancelled");
        return;
    }

    if (!sidebar.delete_path(path))
    {
        Messages::error(std::format("Could not delete {}", path.filename().string()));
        return;
    }

    close_buffers_under(path);
    Messages::info(std::format("Deleted {}", path.filename().string()));
    file_picker.reindex();
}
