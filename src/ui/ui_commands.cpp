#include "ui_impl.hpp"

void UI::register_actions()
{
    register_workbench_actions();
    register_nav_actions();
}

void UI::register_workbench_actions()
{
    commands.register_command("noni.mode.normal", [this]() { return_to_normal(); });

    commands.register_command("workbench.action.files.save", [this]() {
        if (!core.buffers().has_tabs())
            return;
        (void)save_active_buffer();
    });

    commands.register_command("workbench.action.closeActiveEditor", [this]() {
        close_active_tab(false);
    });

    commands.register_command("workbench.action.nextEditor", [this]() {
        core.buffers().next_tab();
        sync_active_tab();
    });

    commands.register_command("workbench.action.previousEditor", [this]() {
        core.buffers().prev_tab();
        sync_active_tab();
    });

    commands.register_command("noni.focus.toggleSidebar", [this]() {
        if (!sidebar_visible)
        {
            open_explorer_view(true);
            return;
        }
        if (focus == Focus::Sidebar || focus == Focus::Search)
            return_to_normal();
        else if (side_view == SideView::Search)
        {
            focus = Focus::Search;
            search_panel.set_focused(true);
            editor.enter_normal_mode();
            keys.clear_chord();
        }
        else
        {
            focus = Focus::Sidebar;
            editor.enter_normal_mode();
            keys.clear_chord();
        }
    });

    commands.register_command("noni.focus.sidebar", [this]() {
        open_sidebar(true);
    });

    commands.register_command("noni.focus.editor", [this]() {
        return_to_normal();
    });

    commands.register_command("workbench.action.toggleSidebarVisibility", [this]() {
        toggle_sidebar();
    });

    commands.register_command("workbench.action.closeSidebar", [this]() {
        close_sidebar();
    });

    commands.register_command("workbench.view.explorer", [this]() {
        open_explorer_view(true);
    });

    commands.register_command("workbench.view.search", [this]() {
        open_search_view(true);
    });

    commands.register_command("workbench.action.findInFiles", [this]() {
        open_search_view(true);
    });

    commands.register_command("noni.command.open", [this]() {
        command_line.open();
        focus = Focus::Command;
        keys.clear_chord();
        resize();
    });

    commands.register_command("workbench.action.quit", [this]() {
        ex_quit(false);
    });

    commands.register_command("workbench.action.quickOpen", [this]() {
        open_file_search();
    });

    commands.register_command("noni.search.files", [this]() {
        open_file_search();
    });

    commands.register_command(Commands::ShowAllEditors, [this]() {
        open_buffer_search();
    });

    commands.register_command(Commands::OpenWorkspace, [this]() {
        open_workspace_prompt();
    });

    commands.register_command(Commands::RevealInExplorer, [this]() {
        if (!core.buffers().has_tabs())
            return;
        const fs::path path = editor.get_buffer().get_buffer_path();
        if (path.empty())
        {
            Messages::info("No file path to reveal");
            return;
        }
        open_explorer_view(true);
        sidebar.reveal_path(path);
    });

    commands.register_command("workbench.action.terminal.toggle", [this]() {
        toggle_terminal();
    });

    commands.register_command("workbench.action.terminal.focus", [this]() {
        open_terminal(true);
    });

    commands.register_command(Commands::TerminalClear, [this]() {
        if (!terminal_visible)
            open_terminal(false);
        terminal.clear_screen();
    });

    commands.register_command(Commands::TerminalScrollUp, [this]() {
        if (!terminal_visible)
            return;
        terminal.scroll_up();
    });

    commands.register_command(Commands::TerminalScrollDown, [this]() {
        if (!terminal_visible)
            return;
        terminal.scroll_down();
    });

    commands.register_command(Commands::TerminalKill, [this]() {
        terminal.stop();
        Messages::info("Terminal process killed");
    });

    commands.register_command("editor.action.clipboardPasteAction", [this]() {
        if (focus != Focus::Editor || !core.buffers().has_tabs())
            return;
        if (!editor.paste_clipboard())
            Messages::warning("Clipboard empty (need wl-paste or xclip)");
    });
}
