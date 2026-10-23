#include "ui_impl.hpp"

void UI::open_sidebar_prompt(PromptIntent intent)
{
    if (command_line.is_active())
        command_line.close();
    if (confirm_prompt.is_active())
        confirm_prompt.close();
    if (file_picker.is_active())
        file_picker.close();

    keys.clear_chord();
    prompt_intent = intent;

    switch (intent)
    {
    case PromptIntent::AddFile:
        input_prompt.open("New file: ");
        break;
    case PromptIntent::AddFolder:
        input_prompt.open("New folder: ");
        break;
    case PromptIntent::Rename:
    {
        const fs::path sel = sidebar.get_selected_path();
        if (sel.empty())
        {
            prompt_intent = PromptIntent::None;
            Messages::error("Nothing to rename");
            return;
        }
        input_prompt.open("Rename: ", sel.filename().string());
        break;
    }
    case PromptIntent::OpenWorkspace:
        input_prompt.open("Open workspace: ", project_root().string());
        break;
    case PromptIntent::RenameSymbol:
        input_prompt.open("Rename to: ");
        break;
    case PromptIntent::WorkspaceSymbolQuery:
        input_prompt.open("Workspace symbol: ");
        break;
    case PromptIntent::None:
        return;
    }

    focus = Focus::Prompt;
    resize();
}

void UI::resolve_sidebar_prompt()
{
    const PromptIntent intent = prompt_intent;
    const std::string name = StrUtils::trim(input_prompt.get_input());
    input_prompt.close();
    prompt_intent = PromptIntent::None;

    if (intent == PromptIntent::OpenWorkspace)
    {
        focus = Focus::Editor;
        resize();
        if (name.empty())
        {
            Messages::info("Cancelled");
            return;
        }
        std::error_code ec;
        fs::path path = name;
        if (!fs::exists(path, ec))
        {
            Messages::error(std::format("Path not found: {}", name));
            return;
        }
        apply_workspace_root(path, true);
        return;
    }

    if (intent == PromptIntent::RenameSymbol)
    {
        focus = Focus::Editor;
        resize();
        if (name.empty())
        {
            Messages::info("Cancelled");
            return;
        }
        Buffer *b = core.active_buffer();
        if (!b)
            return;
        const Cursor c = editor.get_cursor();
        if (core.lsp().request_rename(*b, c.line, c.column, name) <= 0)
            Messages::info("Rename unavailable (no LSP)");
        else
            Messages::info(std::format("Renaming to {}…", name));
        return;
    }

    if (intent == PromptIntent::WorkspaceSymbolQuery)
    {
        focus = Focus::Editor;
        resize();
        Buffer *b = core.active_buffer();
        if (!b)
            return;
        if (core.lsp().request_workspace_symbols(*b, name) <= 0)
            Messages::info("Workspace symbols unavailable (no LSP)");
        else
            Messages::info("Searching workspace symbols…");
        return;
    }

    focus = Focus::Sidebar;
    resize();

    if (name.empty())
    {
        Messages::info("Cancelled");
        return;
    }

    bool ok = false;
    switch (intent)
    {
    case PromptIntent::AddFile:
        ok = sidebar.create_file_here(name);
        if (ok)
        {
            const fs::path path = sidebar.get_selected_path();
            Messages::info(std::format("Created {}", name));
            file_picker.reindex();
            if (!path.empty() && fs::is_regular_file(path))
            {
                core.buffers().open_file(path);
                note_opened_file(path);
                sync_active_tab();
            }
        }
        else
            Messages::error(std::format("Could not create file: {}", name));
        break;
    case PromptIntent::AddFolder:
        ok = sidebar.create_folder_here(name);
        if (ok)
        {
            Messages::info(std::format("Created folder {}", name));
            file_picker.reindex();
        }
        else
            Messages::error(std::format("Could not create folder: {}", name));
        break;
    case PromptIntent::Rename:
    {
        const fs::path before = sidebar.get_selected_path();
        ok = sidebar.rename_selected(name);
        if (ok)
        {
            const fs::path after = sidebar.get_selected_path();
            Messages::info(std::format("Renamed to {}", name));
            remap_buffer_path(before, after);
            file_picker.reindex();
            sync_active_tab();
        }
        else
            Messages::error(std::format("Could not rename to {}", name));
        break;
    }
    case PromptIntent::OpenWorkspace:
    case PromptIntent::RenameSymbol:
    case PromptIntent::WorkspaceSymbolQuery:
    case PromptIntent::None:
        break;
    }
}
