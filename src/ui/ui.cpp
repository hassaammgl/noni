#include "ui_impl.hpp"

UI::UI(const fs::path file_path = "")
{
    Background::instance().start(2);

    const fs::path hint = file_path.empty() ? fs::current_path() : file_path;
    core.workspace().open(hint);
    const fs::path workspace = core.workspace().root();

    std::error_code ec;
    if (!file_path.empty() && fs::is_directory(file_path, ec))
    {
        Logger::info(std::format("Opening workspace folder: {}", file_path.string()));
        if (!restore_session_if_available(true))
            core.buffers().open_untitled();
    }
    else if (file_path.empty())
    {
        Logger::info("No file path provided");
        if (!restore_session_if_available(true))
            core.buffers().open_untitled();
    }
    else
    {
        Logger::info(std::format("Opening file: {}", file_path.string()));
        core.buffers().open_file(file_path);
        note_opened_file(file_path);
        // Still hydrate recent list / UI flags from session without replacing tabs.
        (void)restore_session_if_available(false);
    }

    apply_workspace_root(workspace, false);
    buffer_picker.bind(&core.buffers());

    tab_bar.set_manager(&core.buffers());
    editor.bind_core(&core);
    editor.set_tab_switch_handlers(
        [this]() {
            core.buffers().next_tab();
            sync_active_tab();
        },
        [this]() {
            core.buffers().prev_tab();
            sync_active_tab();
        });
    editor.set_search_handlers(
        [this]() {
            command_line.open('/');
            focus = Focus::Command;
            keys.clear_chord();
            resize();
        },
        [this]() {
            command_line.open('?');
            focus = Focus::Command;
            keys.clear_chord();
            resize();
        });
    sync_active_tab();
    load_config();
    // Config may have enabled LSP servers after first attach attempt.
    if (core.buffers().has_tabs())
        core.attach_lsp_document(core.buffers().active().buffer());
    init();
    register_actions();

    extensions.add(make_hello_extension());
    extensions.activate_all(commands, keys, core, config);
    Logger::info(extensions.status_summary());

    std::vector<CommandId> unbound;
    std::vector<CommandId> unknown;
    commands.diagnose(keys.bound_commands(), &unbound, &unknown);
    std::sort(unbound.begin(), unbound.end());
    std::sort(unknown.begin(), unknown.end());
    for (const auto &id : unknown)
        Logger::warning(std::format("keybinding references unknown command: {}", id));

    std::string unbound_list;
    for (std::size_t i = 0; i < unbound.size(); ++i)
    {
        if (i)
            unbound_list += ", ";
        unbound_list += unbound[i];
    }
    Logger::info(std::format(
        "loaded {} bindings{}, {} commands registered, unbound: {}",
        config.keybindings.size(),
        config.loaded_from.empty() ? "" : std::format(" from {}", config.loaded_from),
        commands.known().size(),
        unbound_list.empty() ? "(none)" : unbound_list));

    if (!config.load_message.empty())
        statusbar.set_echo(config.load_message);

    queue_recovery_prompts();
}

UI::~UI()
{
    save_session();
    extensions.deactivate_all();
    fs_watcher_.stop();
    terminal.stop();
    core.lsp().shutdown_all();
    Background::instance().stop();
    endwin();
    Logger::debug("UI destructor called");
}

void UI::run()
{
    Logger::info("UI main loop started");
    sync_fs_watches();
    while (running)
    {
        file_picker.poll();
        search_panel.poll();
        terminal.poll();
        poll_fs_events();
        if (sidebar.poll() && core.buffers().has_tabs())
        {
            const fs::path path = core.buffers().active().buffer().get_buffer_path();
            if (!path.empty())
                sidebar.reveal_path(path);
        }
        render();
        handle_inputs();
    }
    Logger::info("UI main loop stopped");
}

void UI::request_quit()
{
    save_session();
    running = false;
}


CommandLine &UI::get_command_line()
{
    return command_line;
}

MessagesPanel &UI::get_messages_panel()
{
    return messages_panel;
}

Editor &UI::get_editor()
{
    return editor;
}

BufferManager &UI::get_buffers()
{
    return core.buffers();
}

Dimentions UI::get_editor_dim() const
{
    Dimentions d;
    getmaxyx(stdscr, d.height, d.width);
    return d;
}
