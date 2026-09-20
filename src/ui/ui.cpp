#include <ui/ui.hpp>
#include <utils/async.hpp>
#include <ui/theme.hpp>
#include <editor/ex_commands.hpp>
#include <editor/buffer_search.hpp>
#include <commands/command.hpp>
#include <scm/scm_git.hpp>
#include <lsp/lsp_service.hpp>
#include <extensions/builtin_hello.hpp>
#include <help/help_docs.hpp>
#include <lsp/lsp_edits.hpp>
#include <workspace/workspace.hpp>
#include <workspace/session.hpp>
#include <workspace/recovery.hpp>
#include <utils/fs_watcher.hpp>
#include <syntax/grammar_installer.hpp>
#include <lsp/lsp_installer.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <utils/str.hpp>
#include <utils/text_metrics.hpp>
#include <algorithm>
#include <cstdint>
#include <csignal>
#include <chrono>
#include <fstream>
#include <locale>
#include <format>
#include <map>
#include <regex>
#include <termios.h>
#include <unistd.h>
#include <utils/fs.hpp>
#include <vector>

namespace
{
    InputContext input_context_for(Focus focus, EditorMode mode, bool search_in_text_field)
    {
        switch (focus)
        {
        case Focus::Command:
            return InputContext::CommandLine;
        case Focus::Prompt:
            return InputContext::PromptInput;
        case Focus::FileSearch:
        case Focus::BufferSearch:
        case Focus::LspPicker:
        case Focus::Completion:
            return InputContext::Picker;
        case Focus::Terminal:
            return InputContext::Terminal;
        case Focus::Confirm:
            return InputContext::Confirm;
        case Focus::Messages:
            return InputContext::Messages;
        case Focus::Sidebar:
            return InputContext::Sidebar;
        case Focus::Search:
            return search_in_text_field ? InputContext::SearchText : InputContext::SearchResults;
        case Focus::Editor:
        default:
            if (mode == EditorMode::Insert)
                return InputContext::EditorInsert;
            if (mode == EditorMode::Visual || mode == EditorMode::VisualLine)
                return InputContext::EditorVisual;
            return InputContext::EditorNormal;
        }
    }
}

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

void UI::init()
{
    setlocale(LC_ALL, "");
    initscr();

    raw();
    noecho();
    set_escdelay(config.esc_delay_ms > 0 ? config.esc_delay_ms : 25);
    signal(SIGINT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    termios term{};
    if (tcgetattr(STDIN_FILENO, &term) == 0)
    {
        term.c_iflag &= static_cast<tcflag_t>(~(IXON | IXOFF | IXANY));
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
    }

    keypad(stdscr, TRUE);
    meta(stdscr, TRUE);
    timeout(50); // let async work (index/git/scan) refresh UI without keypress
    curs_set(0);

    if (has_colors())
        Theme::init();

    wbkgd(stdscr, COLOR_PAIR(Theme::Editor));
    erase();
    refresh();

    sidebar_width = config.sidebar_width;
    line_number_width = config.line_number_width;

    resize();
}

void UI::load_config()
{
    config = AppConfig::load();
    keys.load(config);
    GrammarInstaller::set_auto_install(config.syntax_auto_install);
    LspInstaller::set_auto_install(config.lsp_auto_install);

    std::vector<LspServerConfig> servers;
    std::vector<std::string> lsp_bins;
    servers.reserve(config.lsp_servers.size());
    for (const auto &s : config.lsp_servers)
    {
        LspServerConfig c;
        c.language = s.language;
        c.command = s.command;
        c.root_markers = s.root_markers;
        if (!c.command.empty())
            lsp_bins.push_back(c.command[0]);
        servers.push_back(std::move(c));
    }
    core.lsp().set_server_configs(std::move(servers));
    LspInstaller::set_workspace_root(project_root());
    LspInstaller::request_all(lsp_bins);

    terminal_height = std::max(5, config.terminal.height);
    TerminalSessionConfig tcfg;
    tcfg.shell = config.terminal.shell;
    tcfg.scrollback = std::max(0, config.terminal.scrollback);
    tcfg.cwd = project_root().string();
    terminal.apply_config(tcfg);
}

void UI::register_actions()
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

    commands.register_command("editor.action.undo", [this]() {
        if (focus != Focus::Editor || !core.buffers().has_tabs())
            return;
        if (!editor.undo())
            Messages::info("Nothing to undo");
    });

    commands.register_command("editor.action.goToFileStart", [this]() {
        if (focus != Focus::Editor || !core.buffers().has_tabs())
            return;
        editor.set_cursor_position(0, 0);
    });

    commands.register_command("editor.action.redo", [this]() {
        if (focus != Focus::Editor || !core.buffers().has_tabs())
            return;
        if (!editor.redo())
            Messages::info("Nothing to redo");
    });

    commands.register_command(Commands::SplitVertical, [this]() {
        if (core.split_vertical())
            Messages::info("Vertical split");
    });
    commands.register_command(Commands::SplitHorizontal, [this]() {
        if (core.split_horizontal())
            Messages::info("Horizontal split");
    });
    commands.register_command(Commands::CloseWindow, [this]() {
        if (!core.close_window())
            Messages::info("No split to close");
    });
    commands.register_command(Commands::FocusLeft, [this]() { core.focus_left(); });
    commands.register_command(Commands::FocusRight, [this]() { core.focus_right(); });
    commands.register_command(Commands::FocusUp, [this]() { core.focus_up(); });
    commands.register_command(Commands::FocusDown, [this]() { core.focus_down(); });
    commands.register_command(Commands::ResizeLeft, [this]() { core.resize_left(); });
    commands.register_command(Commands::ResizeRight, [this]() { core.resize_right(); });
    commands.register_command(Commands::ResizeUp, [this]() { core.resize_up(); });
    commands.register_command(Commands::ResizeDown, [this]() { core.resize_down(); });

    commands.register_command(Commands::SearchNext, [this]() {
        const std::string err = core.search_next(false);
        if (!err.empty())
            Messages::info(err);
    });
    commands.register_command(Commands::SearchPrevious, [this]() {
        const std::string err = core.search_next(true);
        if (!err.empty())
            Messages::info(err);
    });
    commands.register_command(Commands::SearchReplace, [this]() {
        // Replace current with empty unless replace text from panel; buffer search uses "".
        // Prefer panel replace text when search panel has one; else no-op message.
        const std::string &r = search_panel.get_replace();
        const std::string err = core.replace_current(r);
        if (!err.empty())
            Messages::warning(err);
        else
            Messages::info("Replaced match");
    });
    commands.register_command(Commands::SearchReplaceAll, [this]() {
        if (focus == Focus::Search)
        {
            apply_replace_all();
            return;
        }
        const std::string &r = search_panel.get_replace();
        const std::string err = core.replace_all(r);
        if (!err.empty())
            Messages::warning(err);
        else
            Messages::info("Replaced all matches in buffer");
    });

    commands.register_command(Commands::ScmRefresh, [this]() {
        refresh_scm(project_root());
        Messages::info("SCM refresh requested");
    });

    commands.register_command(Commands::ScmShowStatus, [this]() {
        sync_scm_ui();
        const auto snap = core.scm().snapshot();
        if (!snap.is_repo)
        {
            Messages::info("Not a git repository");
            return;
        }
        std::string msg = std::format("Git {} · {} files", snap.branch, snap.files.size());
        if (snap.ahead >= 0 || snap.behind >= 0)
            msg += std::format(" · ↑{} ↓{}", std::max(0, snap.ahead), std::max(0, snap.behind));
        if (Buffer *b = core.active_buffer())
        {
            if (auto st = core.scm().status_for(b->get_buffer_path()))
                msg += std::format(" · [{}{}]", st->xy[0], st->xy[1]);
            else
                msg += " · [clean]";
        }
        Messages::info(msg);
    });

    commands.register_command(Commands::ScmStage, [this]() { scm_stage_active(); });
    commands.register_command(Commands::ScmUnstage, [this]() { scm_unstage_active(); });
    commands.register_command(Commands::ScmDiscard, [this]() { scm_discard_active_confirm(); });
    commands.register_command(Commands::ScmShowDiff, [this]() { scm_show_diff_summary(); });
    commands.register_command(Commands::ScmRefreshDiff, [this]() {
        scm_diff_path_.clear();
        scm_diff_lines_ = -1;
        sync_scm_diff();
        Messages::info("Diff refresh requested");
    });

    commands.register_command(Commands::LspShowStatus, [this]() {
        core.lsp().pump();
        Messages::info(core.lsp().status_summary());
    });

    commands.register_command(Commands::LspInstall, [this]() {
        open_lsp_install_picker();
    });

    commands.register_command(Commands::LspRestart, [this]() {
        core.lsp().shutdown_all();
        core.lsp().set_workspace_root(project_root());
        if (Buffer *b = core.active_buffer())
            core.attach_lsp_document(*b);
        Messages::info("LSP restarted");
    });

    commands.register_command(Commands::TriggerSuggest, [this]() {
        trigger_completion();
    });

    commands.register_command(Commands::GotoDefinition, [this]() { request_lsp_definition(); });
    commands.register_command(Commands::GotoDeclaration, [this]() { request_lsp_declaration(); });
    commands.register_command(Commands::GotoTypeDefinition, [this]() { request_lsp_type_definition(); });
    commands.register_command(Commands::FindReferences, [this]() { request_lsp_references(); });
    commands.register_command(Commands::DocumentSymbols, [this]() { request_lsp_document_symbols(); });
    commands.register_command(Commands::WorkspaceSymbols, [this]() {
        request_lsp_workspace_symbols_prompt();
    });
    commands.register_command(Commands::RenameSymbol, [this]() { request_lsp_rename_prompt(); });
    commands.register_command(Commands::CodeAction, [this]() { request_lsp_code_actions(); });

    commands.register_command(Commands::SessionSave, [this]() {
        save_session();
        Messages::info("Session saved");
    });
    commands.register_command(Commands::SessionRestore, [this]() {
        if (restore_session_if_available(true))
        {
            sync_active_tab();
            Messages::info("Session restored");
        }
        else
            Messages::warning("No session to restore");
    });

    commands.register_command("extension.showStatus", [this]() {
        Messages::info(extensions.status_summary());
    });
}

std::string UI::when_context() const
{
    std::string ctx;
    if (focus == Focus::Editor)
        ctx += "editorFocus ";
    if (focus == Focus::Sidebar)
        ctx += "sidebarFocus ";
    if (focus == Focus::Search)
        ctx += "searchFocus ";
    if (focus == Focus::Command)
        ctx += "commandFocus ";
    if (focus == Focus::Messages)
        ctx += "messagesFocus ";
    if (focus == Focus::FileSearch)
        ctx += "fileSearchFocus ";
    if (focus == Focus::BufferSearch)
        ctx += "bufferSearchFocus ";
    if (focus == Focus::LspPicker)
        ctx += "lspPickerFocus ";
    if (focus == Focus::Completion)
        ctx += "completionFocus ";
    if (focus == Focus::Confirm)
        ctx += "confirmFocus ";
    if (focus == Focus::Prompt)
        ctx += "promptFocus ";
    if (focus == Focus::Terminal)
        ctx += "terminalFocus ";
    if (editor.get_mode() == EditorMode::Normal)
        ctx += "normalMode ";
    if (editor.get_mode() == EditorMode::Insert)
        ctx += "insertMode ";
    if (editor.get_mode() == EditorMode::Visual || editor.get_mode() == EditorMode::VisualLine)
        ctx += "visualMode ";
    if (sidebar_visible)
        ctx += "sidebarVisible ";
    else
        ctx += "sidebarHidden ";
    if (terminal_visible)
        ctx += "terminalVisible ";
    else
        ctx += "terminalHidden ";
    return StrUtils::trim(ctx);
}

void UI::set_sidebar_visible(bool visible)
{
    if (sidebar_visible == visible)
        return;

    sidebar_visible = visible;
    if (!sidebar_visible && (focus == Focus::Sidebar || focus == Focus::Search))
    {
        focus = Focus::Editor;
        editor.enter_normal_mode();
    }
    keys.clear_chord();
    resize();
}

void UI::toggle_sidebar()
{
    set_sidebar_visible(!sidebar_visible);
}

void UI::open_sidebar(bool focus_sidebar)
{
    open_explorer_view(focus_sidebar);
}

int UI::effective_sidebar_width() const
{
    if (!sidebar_visible)
        return 0;
    if (side_view == SideView::Search)
        return std::max(sidebar_width, 36);
    return sidebar_width;
}

void UI::open_explorer_view(bool focus_explorer)
{
    sidebar_visible = true;
    side_view = SideView::Explorer;
    search_panel.close();
    if (focus_explorer)
    {
        focus = Focus::Sidebar;
        editor.enter_normal_mode();
    }
    keys.clear_chord();
    resize();
}

void UI::open_search_view(bool focus_search)
{
    sidebar_visible = true;
    side_view = SideView::Search;
    search_panel.set_root(project_root());
    search_panel.open();
    if (focus_search)
    {
        focus = Focus::Search;
        editor.enter_normal_mode();
    }
    keys.clear_chord();
    resize();
}

void UI::apply_replace_all()
{
    const std::string q = search_panel.get_query();
    const std::string r = search_panel.get_replace();
    if (q.empty())
    {
        Messages::error("Nothing to replace");
        return;
    }

    auto matches = search_panel.results();
    if (matches.empty())
    {
        Messages::error("No matches");
        return;
    }

    const auto opts = search_panel.get_options();

    auto find_open_buffer = [&](const fs::path &path) -> Buffer * {
        return core.buffers().find_buffer_by_path(path);
    };

    // Group by path.
    std::map<fs::path, std::vector<TextMatch>> by_path;
    for (const auto &m : matches)
        by_path[m.path].push_back(m);

    int files_touched = 0;
    int replacements = 0;

    for (auto &[path, file_matches] : by_path)
    {
        std::sort(file_matches.begin(), file_matches.end(), [](const TextMatch &a, const TextMatch &b) {
            if (a.line != b.line)
                return a.line > b.line;
            return a.column > b.column;
        });

        if (Buffer *buf = find_open_buffer(path))
        {
            BufferSearchQuery bq;
            bq.pattern = q;
            bq.options = opts;
            auto buffer_matches = BufferSearch::find_all(buf->lines(), bq);
            if (buffer_matches.empty())
                continue;

            std::sort(buffer_matches.begin(), buffer_matches.end(),
                      [](const BufferSearchMatch &a, const BufferSearchMatch &b) {
                          if (a.start.line != b.start.line)
                              return a.start.line > b.start.line;
                          return a.start.column > b.start.column;
                      });

            Cursor cur{.line = 0, .column = 0};
            if (core.active_buffer() == buf && core.active_window())
                cur = core.active_window()->cursor();

            buf->begin_edit(cur.line, cur.column);
            for (const auto &m : buffer_matches)
            {
                buf->delete_range(m.start.line, m.start.column, m.end.line, m.end.column);
                (void)buf->insert_text(m.start.line, m.start.column, r);
                ++replacements;
            }
            buf->end_edit(cur.line, cur.column);
            ++files_touched;
            continue;
        }

        // File not open: atomic FS write (no Buffer history).
        std::ifstream in(path);
        if (!in)
            continue;
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(line);
        }

        bool dirty = false;
        if (opts.use_regex)
        {
            try
            {
                auto flags = std::regex::ECMAScript;
                if (!opts.match_case)
                    flags |= std::regex::icase;
                std::regex re(q, flags);
                for (auto &row : lines)
                {
                    const std::string before = row;
                    row = std::regex_replace(row, re, r);
                    if (row != before)
                    {
                        ++replacements;
                        dirty = true;
                    }
                }
            }
            catch (...)
            {
                continue;
            }
        }
        else
        {
            for (const auto &m : file_matches)
            {
                if (m.line < 0 || m.line >= static_cast<int>(lines.size()))
                    continue;
                std::string &row = lines[static_cast<std::size_t>(m.line)];
                if (m.column < 0 ||
                    m.column + static_cast<int>(q.size()) > static_cast<int>(row.size()))
                    continue;
                std::string span = row.substr(static_cast<std::size_t>(m.column), q.size());
                const bool ok = opts.match_case
                                    ? (span == q)
                                    : (StrUtils::to_lower(span) == StrUtils::to_lower(q));
                if (!ok)
                    continue;
                row.replace(static_cast<std::size_t>(m.column), q.size(), r);
                ++replacements;
                dirty = true;
            }
        }

        if (dirty)
        {
            FS fs;
            if (fs.write_file(path, lines))
                ++files_touched;
        }
    }

    sync_active_tab();
    Messages::info(std::format("Replaced {} matches across {} files", replacements, files_touched));
}

void UI::close_sidebar()
{
    set_sidebar_visible(false);
}

fs::path UI::project_root() const
{
    if (core.workspace().has_root())
        return core.workspace().root();
    fs::path root = sidebar.get_project_path();
    if (!root.empty())
        return root;
    if (core.buffers().has_tabs())
    {
        const auto path = core.buffers().active().buffer().get_buffer_path();
        if (!path.empty())
            return path.parent_path().empty() ? fs::current_path() : path.parent_path();
    }
    return fs::current_path();
}

void UI::refresh_scm(const fs::path &hint)
{
    const fs::path root = Workspace::detect_root(hint.empty() ? project_root() : hint);
    if (!core.workspace().has_root() || core.workspace().root() != root)
        core.workspace().set_root(root);
    core.scm().set_workspace_root(root);
    core.scm().request_refresh();
    scm_gen_seen_ = 0;
    scm_diff_path_.clear();
    scm_diff_lines_ = -1;
    scm_diff_gen_seen_ = 0;
    core.lsp().set_workspace_root(root);
    GrammarInstaller::set_workspace_root(root);
}

void UI::sync_scm_diff()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const fs::path path = b->get_buffer_path();
    if (path.empty())
        return;

    const int lines = static_cast<int>(b->lines().size());
    if (path != scm_diff_path_ || lines != scm_diff_lines_ ||
        core.scm().diff_generation() != scm_diff_gen_seen_)
    {
        // Request when path/lines changed; generation bump means cache updated.
        if (path != scm_diff_path_ || lines != scm_diff_lines_)
        {
            scm_diff_path_ = path;
            scm_diff_lines_ = lines;
            core.scm().request_file_diff(path, lines);
        }
        scm_diff_gen_seen_ = core.scm().diff_generation();
    }
}

void UI::scm_stage_active()
{
    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
    {
        Messages::warning("No file to stage");
        return;
    }
    std::string err;
    if (!core.scm().stage(b->get_buffer_path(), &err))
    {
        Messages::error(err.empty() ? "Stage failed" : err);
        return;
    }
    scm_diff_path_.clear();
    sync_scm_diff();
    Messages::info(std::format("Staged {}", b->get_buffer_path().filename().string()));
}

void UI::scm_unstage_active()
{
    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
    {
        Messages::warning("No file to unstage");
        return;
    }
    std::string err;
    if (!core.scm().unstage(b->get_buffer_path(), &err))
    {
        Messages::error(err.empty() ? "Unstage failed" : err);
        return;
    }
    scm_diff_path_.clear();
    sync_scm_diff();
    Messages::info(std::format("Unstaged {}", b->get_buffer_path().filename().string()));
}

void UI::scm_discard_active_confirm()
{
    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
    {
        Messages::warning("No file to discard");
        return;
    }
    if (b->is_dirty())
    {
        Messages::error("Buffer has unsaved changes — save or discard buffer edits first");
        return;
    }
    if (command_line.is_active())
        command_line.close();
    if (input_prompt.is_active())
        input_prompt.close();
    confirm_intent = ConfirmIntent::DiscardGit;
    confirm_prompt.open(std::format(
        "Discard worktree changes to {}? [y/n/esc]",
        b->get_buffer_path().filename().string()));
    focus = Focus::Confirm;
    resize();
}

void UI::resolve_discard_confirm(ConfirmChoice choice)
{
    confirm_prompt.close();
    confirm_intent = ConfirmIntent::None;
    focus = Focus::Editor;
    resize();

    if (choice != ConfirmChoice::Yes)
    {
        Messages::info("Discard cancelled");
        return;
    }

    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
        return;
    if (b->is_dirty())
    {
        Messages::error("Buffer became dirty — discard aborted");
        return;
    }

    const fs::path path = b->get_buffer_path();
    std::string err;
    if (!core.scm().discard_worktree(path, &err))
    {
        Messages::error(err.empty() ? "Discard failed" : err);
        return;
    }

    // Reload from disk (P0-safe load); preserve undo? load() clears history — OK for discard.
    b->load();
    core.attach_lsp_document(*b);
    scm_diff_path_.clear();
    sync_active_tab();
    sync_scm_diff();
    Messages::info(std::format("Discarded changes: {}", path.filename().string()));
}

void UI::scm_show_diff_summary()
{
    sync_scm_diff();
    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
    {
        Messages::info("No file");
        return;
    }
    auto diff = core.scm().file_diff(b->get_buffer_path());
    if (!diff)
    {
        Messages::info("Diff not ready yet — try again");
        core.scm().request_file_diff(b->get_buffer_path(), static_cast<int>(b->lines().size()));
        return;
    }
    if (!diff->ok)
    {
        Messages::warning(diff->error.empty() ? "Diff unavailable" : diff->error);
        return;
    }
    int added = 0, modified = 0, deleted = 0;
    for (const auto &[line, ch] : diff->lines)
    {
        (void)line;
        switch (ch)
        {
        case ScmLineChange::Added:
            ++added;
            break;
        case ScmLineChange::Modified:
            ++modified;
            break;
        case ScmLineChange::Deleted:
            ++deleted;
            break;
        default:
            break;
        }
    }
    Messages::info(std::format(
        "Diff {}: +{} ~{} -{} ({} hunks){}",
        b->get_buffer_path().filename().string(),
        added,
        modified,
        deleted,
        diff->hunks.size(),
        diff->is_untracked ? " [untracked]" : ""));
}

fs::path UI::find_workspace_root(const fs::path &hint) const
{
    return Workspace::detect_root(hint);
}

void UI::note_opened_file(const fs::path &path)
{
    if (path.empty())
        return;
    core.recent().touch(path);
}

SessionState UI::capture_session_state() const
{
    SessionState st;
    st.workspace_root = core.workspace().root().string();
    st.active_index = core.buffers().get_active_index();
    st.sidebar_visible = sidebar_visible;
    st.terminal_visible = terminal_visible;
    for (const auto &tab : core.buffers().get_tabs())
    {
        const fs::path p = tab.buffer().get_buffer_path();
        if (p.empty())
            continue;
        SessionTabState t;
        t.path = p.string();
        t.cursor_line = tab.cursor().line;
        t.cursor_column = tab.cursor().column;
        t.scroll_y = tab.scroll_y();
        t.scroll_x = tab.scroll_x();
        st.tabs.push_back(std::move(t));
    }
    for (const auto &r : core.recent().list())
        st.recent.push_back(r.string());
    return st;
}

void UI::save_session()
{
    const fs::path root = core.workspace().root();
    if (root.empty())
        return;
    std::string err;
    if (!SessionStore::save(root, capture_session_state(), &err))
        Logger::warning(err.empty() ? "session save failed" : err);
}

bool UI::restore_session_if_available(bool allow_replace_tabs)
{
    const fs::path root = core.workspace().root();
    if (root.empty())
        return false;

    std::string err;
    auto st = SessionStore::load(root, &err);
    if (!st)
        return false;

    for (const auto &r : st->recent)
    {
        if (!r.empty())
            core.recent().touch(r);
    }

    sidebar_visible = st->sidebar_visible;
    terminal_visible = st->terminal_visible;

    if (!allow_replace_tabs || st->tabs.empty())
        return false;

    // Close current tabs (untitled only expected at startup).
    while (core.buffers().has_tabs())
    {
        if (!core.buffers().close_active(true))
            break;
        if (!core.buffers().has_tabs())
            break;
    }

    int opened = 0;
    for (const auto &t : st->tabs)
    {
        std::error_code ec;
        if (t.path.empty() || !fs::exists(t.path, ec))
            continue;
        core.buffers().open_file(t.path);
        note_opened_file(t.path);
        EditorTab &tab = core.buffers().active();
        tab.cursor().line = std::max(0, t.cursor_line);
        tab.cursor().column = std::max(0, t.cursor_column);
        tab.scroll_y() = std::max(0, t.scroll_y);
        tab.scroll_x() = std::max(0, t.scroll_x);
        ++opened;
    }

    if (opened == 0)
    {
        core.buffers().open_untitled();
        return false;
    }

    const int idx = std::clamp(st->active_index, 0, static_cast<int>(core.buffers().size()) - 1);
    core.buffers().switch_to(idx);
    session_restored_ = true;
    Logger::info(std::format("Session restored ({} tabs)", opened));
    return true;
}

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

void UI::remap_buffer_path(const fs::path &from, const fs::path &to)
{
    if (from.empty() || to.empty())
        return;
    if (Buffer *b = core.buffers().find_buffer_by_path(from))
        b->set_save_path(to);
}

void UI::close_buffers_under(const fs::path &path)
{
    if (path.empty() || !core.buffers().has_tabs())
        return;
    // Close matching tabs from the end to keep indices stable enough.
    for (int guard = 0; guard < 64 && core.buffers().has_tabs(); ++guard)
    {
        bool closed_any = false;
        const auto &tabs = core.buffers().get_tabs();
        for (int i = static_cast<int>(tabs.size()) - 1; i >= 0; --i)
        {
            const auto p = tabs[static_cast<std::size_t>(i)].buffer().get_buffer_path();
            if (p.empty())
                continue;
            std::error_code ec;
            const bool match = (p == path) ||
                               (fs::is_directory(path, ec) &&
                                p.string().rfind(path.string(), 0) == 0);
            if (!match)
                continue;
            core.buffers().switch_to(i);
            Buffer *b = &core.buffers().active().buffer();
            core.lsp().notify_close(*b);
            core.on_buffer_closed(b);
            core.buffers().close_active(true);
            closed_any = true;
            break;
        }
        if (!closed_any)
            break;
    }
    if (!core.buffers().has_tabs())
        core.buffers().open_untitled();
    sync_active_tab();
}

void UI::apply_workspace_root(const fs::path &hint, bool announce)
{
    core.workspace().open(hint);
    const fs::path root = core.workspace().root();
    sidebar.set_project_path(root);
    search_panel.set_root(root);
    file_picker.warm(root);
    file_picker.reindex();
    refresh_scm(root);
    terminal.set_cwd(root.string());
    sync_fs_watches();
    if (announce)
        Messages::info(std::format("Workspace: {}", root.string()));
}

void UI::sync_fs_watches()
{
    if (!fs_watcher_.start())
        return;

    fs_watcher_.set_workspace(project_root());

    // Track open buffer files for external edit detection.
    if (!core.buffers().has_tabs())
        return;
    for (const auto &tab : core.buffers().get_tabs())
    {
        const fs::path path = tab.buffer().get_buffer_path();
        if (!path.empty())
            fs_watcher_.watch(path);
    }
}

void UI::poll_fs_events()
{
    const auto events = fs_watcher_.poll();
    if (events.empty() && !fs_explorer_dirty_)
        return;

    bool explorer_touch = fs_explorer_dirty_;
    for (const auto &ev : events)
    {
        Logger::debug(std::format(
            "FsWatcher event: {} ({})",
            ev.path.string(),
            ev.is_dir ? "dir" : "file"));

        // Workspace tree changes → debounce explorer + file index refresh.
        const fs::path root = project_root();
        if (!root.empty())
        {
            std::error_code ec;
            const fs::path abs = fs::weakly_canonical(ev.path, ec);
            const std::string rs = root.string();
            const std::string ps = ec ? ev.path.string() : abs.string();
            if (ps == rs || (ps.size() > rs.size() && ps.compare(0, rs.size(), rs) == 0 &&
                             (ps[rs.size()] == '/')))
                explorer_touch = true;
        }

        if (ev.is_dir)
        {
            if (ev.kind == FsEventKind::Created || ev.kind == FsEventKind::Moved)
                fs_watcher_.watch(ev.path);
            continue;
        }

        Buffer *buf = core.buffers().find_buffer_by_path(ev.path);
        if (!buf)
            continue;
        if (!buf->disk_changed())
            continue;

        if (buf->is_dirty())
        {
            if (!buf->external_change_notified())
            {
                buf->mark_external_change_notified();
                Messages::warning(std::format(
                    "{} changed on disk (buffer dirty — :e! to reload)",
                    ev.path.filename().string()));
            }
            continue;
        }

        // Clean buffer: safe auto-reload.
        buf->load();
        buf->clear_external_change_flag();
        Messages::info(std::format("Reloaded {}", ev.path.filename().string()));
        if (core.active_buffer() == buf)
            sync_active_tab();
    }

    if (explorer_touch)
    {
        fs_explorer_dirty_ = true;
        const auto now = std::chrono::steady_clock::now();
        if (fs_explorer_refresh_at_.time_since_epoch().count() == 0)
            fs_explorer_refresh_at_ = now + std::chrono::milliseconds(250);
    }

    if (fs_explorer_dirty_ && std::chrono::steady_clock::now() >= fs_explorer_refresh_at_)
    {
        fs_explorer_dirty_ = false;
        fs_explorer_refresh_at_ = {};
        sidebar.refresh();
        file_picker.reindex();
        Logger::debug("FsWatcher: explorer refreshed");
    }
}

void UI::sync_messages_echo()
{
    const auto [seq, text] = Messages::echo_snapshot();
    if (seq == messages_echo_seen_)
        return;
    messages_echo_seen_ = seq;
    if (!text.empty())
        statusbar.set_echo(text);
}

void UI::goto_lsp_location(const LspLocation &loc)
{
    const fs::path path = LspService::uri_to_path(loc.uri);
    if (path.empty())
    {
        Messages::warning("LSP location has no path");
        return;
    }

    editor.record_jump_from_here();
    core.buffers().open_file(path);
    sync_active_tab();
    note_opened_file(path);

    Buffer &buf = editor.get_buffer();
    const Cursor byte_pos = LspService::lsp_pos_to_cursor(
        buf.lines(), loc.start.line, loc.start.column);
    editor.set_cursor_position(byte_pos.line, byte_pos.column);
    editor.enter_normal_mode();
    focus = Focus::Editor;
    Messages::info(std::format("→ {}:{}", path.filename().string(), byte_pos.line + 1));
}

void UI::apply_lsp_workspace_edit(LspWorkspaceEdit edit)
{
    const int n = LspEdits::apply_workspace_edit(
        core.buffers(),
        std::move(edit),
        [this](Buffer &b) { core.attach_lsp_document(b); });
    sync_active_tab();
    if (n > 0)
        Messages::info(std::format("Applied workspace edit ({} buffer(s))", n));
    else
        Messages::warning("Workspace edit applied nothing");
}

void UI::open_lsp_locations(std::string title, std::vector<LspLocation> locs)
{
    if (locs.empty())
    {
        Messages::info("No locations");
        return;
    }
    if (locs.size() == 1)
    {
        goto_lsp_location(locs.front());
        return;
    }
    lsp_picker_kind_ = LspPickerKind::Locations;
    lsp_picker.open_locations(std::move(title), std::move(locs));
    focus = Focus::LspPicker;
    resize();
}

void UI::open_lsp_symbols(std::string title, std::vector<LspSymbol> syms)
{
    if (syms.empty())
    {
        Messages::info("No symbols");
        return;
    }
    lsp_picker_kind_ = LspPickerKind::Symbols;
    lsp_picker.open_symbols(std::move(title), std::move(syms));
    focus = Focus::LspPicker;
    resize();
}

void UI::open_lsp_actions(std::vector<LspCodeAction> actions)
{
    if (actions.empty())
    {
        Messages::info("No code actions");
        return;
    }
    lsp_picker_kind_ = LspPickerKind::CodeActions;
    lsp_picker.open_actions("code actions", std::move(actions));
    focus = Focus::LspPicker;
    resize();
}

void UI::open_lsp_install_picker()
{
    std::vector<LspCodeAction> items;
    for (const auto &e : LspInstaller::catalog())
    {
        const auto st = LspInstaller::status(e.binary);
        const fs::path path = LspInstaller::resolve(e.binary);
        LspCodeAction a;
        a.title = std::format(
            "[{}] {}  · {} ({})",
            LspInstaller::status_label(st),
            e.binary,
            e.via,
            e.package);
        if (!path.empty())
            a.title += std::format("  → {}", path.string());
        a.kind = "install";
        a.has_command = true;
        a.command = e.binary;
        a.is_preferred = st != LspInstaller::Status::Ready;
        items.push_back(std::move(a));
    }
    if (items.empty())
    {
        Messages::info("No LSP install recipes");
        return;
    }
    lsp_picker_kind_ = LspPickerKind::InstallServers;
    lsp_picker.open_actions("LSP servers (Enter = install)", std::move(items));
    focus = Focus::LspPicker;
    resize();
}

void UI::ex_lsp()
{
    open_lsp_install_picker();
}

void UI::close_lsp_picker(bool accept)
{
    if (!lsp_picker.is_active())
    {
        focus = Focus::Editor;
        return;
    }

    if (!accept)
    {
        lsp_picker.close();
        focus = Focus::Editor;
        resize();
        return;
    }

    if (lsp_picker_kind_ == LspPickerKind::Locations)
    {
        LspLocation loc;
        if (lsp_picker.take_location(loc))
            goto_lsp_location(loc);
        else
            lsp_picker.close();
        focus = Focus::Editor;
        resize();
    }
    else if (lsp_picker_kind_ == LspPickerKind::Symbols)
    {
        LspSymbol sym;
        if (lsp_picker.take_symbol(sym))
            goto_lsp_location(sym.location);
        else
            lsp_picker.close();
        focus = Focus::Editor;
        resize();
    }
    else if (lsp_picker_kind_ == LspPickerKind::InstallServers)
    {
        LspCodeAction act;
        if (lsp_picker.take_action(act) && act.has_command && !act.command.empty())
        {
            if (!LspInstaller::resolve(act.command).empty())
                Messages::info(std::format("LSP `{}` already installed", act.command));
            else
                LspInstaller::request(act.command, true);
        }
        else
        {
            lsp_picker.close();
        }
        focus = Focus::Editor;
        resize();
    }
    else
    {
        LspCodeAction act;
        if (lsp_picker.take_action(act))
        {
            if (act.has_edit)
                apply_lsp_workspace_edit(std::move(act.edit));
            else if (act.has_command)
                Messages::warning(std::format(
                    "Code action command not executed: {}", act.command));
            else
                Messages::info(act.title);
        }
        else
        {
            lsp_picker.close();
        }
        focus = Focus::Editor;
        resize();
    }
}

void UI::poll_lsp_results()
{
    if (auto edit = core.lsp().take_server_apply_edit())
        apply_lsp_workspace_edit(std::move(*edit));

    if (auto locs = core.lsp().take_location_result())
    {
        const char *title = "locations";
        switch (locs->kind)
        {
        case LspLocationList::Kind::Declaration:
            title = "declarations";
            break;
        case LspLocationList::Kind::TypeDefinition:
            title = "type definitions";
            break;
        case LspLocationList::Kind::References:
            title = "references";
            break;
        default:
            title = "definitions";
            break;
        }
        open_lsp_locations(title, std::move(locs->items));
    }

    if (auto syms = core.lsp().take_symbol_result())
    {
        open_lsp_symbols(
            syms->workspace ? "workspace symbols" : "document symbols",
            std::move(syms->items));
    }

    if (auto ren = core.lsp().take_rename_result())
    {
        if (ren->edit.changes.empty())
            Messages::warning("Rename produced no edits");
        else
            apply_lsp_workspace_edit(std::move(ren->edit));
    }

    if (auto acts = core.lsp().take_code_action_result())
        open_lsp_actions(std::move(acts->items));
}

void UI::request_lsp_definition()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_definition(*b, c.line, c.column) <= 0)
        Messages::info("Definition unavailable (no LSP)");
    else
        Messages::info("Finding definition…");
}

void UI::request_lsp_declaration()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_declaration(*b, c.line, c.column) <= 0)
        Messages::info("Declaration unavailable (no LSP)");
    else
        Messages::info("Finding declaration…");
}

void UI::request_lsp_type_definition()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_type_definition(*b, c.line, c.column) <= 0)
        Messages::info("Type definition unavailable (no LSP)");
    else
        Messages::info("Finding type definition…");
}

void UI::request_lsp_references()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_references(*b, c.line, c.column) <= 0)
        Messages::info("References unavailable (no LSP)");
    else
        Messages::info("Finding references…");
}

void UI::request_lsp_document_symbols()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    if (core.lsp().request_document_symbols(*b) <= 0)
        Messages::info("Document symbols unavailable (no LSP)");
    else
        Messages::info("Loading symbols…");
}

void UI::request_lsp_workspace_symbols_prompt()
{
    if (command_line.is_active())
        command_line.close();
    if (lsp_picker.is_active())
        lsp_picker.close();
    keys.clear_chord();
    prompt_intent = PromptIntent::WorkspaceSymbolQuery;
    input_prompt.open("Workspace symbol: ");
    focus = Focus::Prompt;
    resize();
}

void UI::request_lsp_rename_prompt()
{
    if (command_line.is_active())
        command_line.close();
    if (lsp_picker.is_active())
        lsp_picker.close();
    keys.clear_chord();
    prompt_intent = PromptIntent::RenameSymbol;
    input_prompt.open("Rename to: ");
    focus = Focus::Prompt;
    resize();
}

void UI::request_lsp_code_actions()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_code_actions(*b, c, c) <= 0)
        Messages::info("Code actions unavailable (no LSP)");
    else
        Messages::info("Loading code actions…");
}

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

void UI::trigger_completion()
{
    if (!core.buffers().has_tabs())
        return;
    if (completion_picker.is_active())
        close_completion(false);

    Buffer &buf = editor.get_buffer();
    const Cursor c = editor.get_cursor();
    core.lsp().cancel_completion();
    const int id = core.lsp().request_completion(buf, c.line, c.column);
    if (id < 0)
    {
        Messages::info("LSP still starting — try Ctrl+Space again in a second");
        return;
    }
    if (id == 0)
    {
        Messages::info("Completion unavailable (no LSP for this buffer)");
        return;
    }
    Messages::info("Completing…");
}

void UI::close_completion(bool accept)
{
    CompletionItem item;
    const bool took = accept && completion_picker.take_selection(item);
    if (!took)
        completion_picker.close();
    core.lsp().cancel_completion();
    focus = Focus::Editor;
    keys.clear_chord();
    resize();

    if (took)
    {
        if (editor.apply_completion(item))
            Messages::info(std::format("Inserted {}", item.label));
        else
            Messages::warning("Failed to insert completion");
    }
}

void UI::poll_completion_result()
{
    auto result = core.lsp().take_completion_result();
    if (!result)
        return;

    if (result->items.empty())
    {
        Messages::info("No completions");
        return;
    }

    if (file_picker.is_active())
        file_picker.close();
    if (command_line.is_active())
    {
        command_line.clear_input();
        command_line.close();
    }

    completion_picker.open(std::move(*result));
    focus = Focus::Completion;
    keys.clear_chord();
    resize();
}

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

void UI::return_to_normal()
{
    keys.clear_chord();

    if (command_line.is_active())
    {
        command_line.clear_input();
        command_line.close();
    }

    if (messages_panel.is_active())
        messages_panel.close();

    if (file_picker.is_active())
        file_picker.close();

    if (buffer_picker.is_active())
        buffer_picker.close();

    if (lsp_picker.is_active())
        lsp_picker.close();

    if (completion_picker.is_active())
        completion_picker.close();
    core.lsp().cancel_pending();

    if (confirm_prompt.is_active())
    {
        confirm_prompt.close();
        confirm_intent = ConfirmIntent::None;
        pending_delete_path.clear();
    }

    if (input_prompt.is_active())
    {
        input_prompt.close();
        prompt_intent = PromptIntent::None;
    }

    terminal.set_focused(false);
    focus = Focus::Editor;
    editor.enter_normal_mode();
    resize();
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
    if (editor.get_buffer().get_buffer_path().empty())
    {
        Messages::error("E32: No file name (use :w <path>)");
        return false;
    }

    if (editor.get_buffer().has_load_error())
    {
        Messages::error(std::format(
            "E13: Cannot save — file was not loaded ({})",
            editor.get_buffer().get_load_error()));
        return false;
    }

    if (!editor.get_buffer().save())
    {
        Messages::error("E212: Can't open file for writing");
        return false;
    }

    statusbar.set_filename(core.buffers().active().display_name());
    Messages::info(std::format("\"{}\" written", core.buffers().active().display_name()));
    clear_recovery_for_buffer(editor.get_buffer());
    core.scm().request_refresh();
    core.lsp().notify_save(editor.get_buffer());
    core.notify_buffer_saved(editor.get_buffer());
    return true;
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

void UI::update_statusbar_mode()
{
    switch (focus)
    {
    case Focus::Command:
        statusbar.set_mode("COMMAND");
        break;
    case Focus::Messages:
        statusbar.set_mode("MESSAGES");
        break;
    case Focus::Sidebar:
        statusbar.set_mode("SIDEBAR");
        break;
    case Focus::Search:
        statusbar.set_mode("SEARCH");
        break;
    case Focus::FileSearch:
        statusbar.set_mode("FILES");
        break;
    case Focus::BufferSearch:
        statusbar.set_mode("BUFFERS");
        break;
    case Focus::LspPicker:
        statusbar.set_mode("LSP");
        break;
    case Focus::Completion:
        statusbar.set_mode("COMPLETE");
        break;
    case Focus::Confirm:
        statusbar.set_mode("CONFIRM");
        break;
    case Focus::Prompt:
        statusbar.set_mode("INPUT");
        break;
    case Focus::Terminal:
        statusbar.set_mode("TERMINAL");
        break;
    case Focus::Editor:
        statusbar.set_mode(editor.get_mode_label());
        break;
    }
}

void UI::update_cursor_visibility()
{
    if (focus == Focus::Command || focus == Focus::FileSearch ||
        focus == Focus::BufferSearch || focus == Focus::LspPicker ||
        focus == Focus::Completion || focus == Focus::Confirm ||
        focus == Focus::Prompt || focus == Focus::Terminal || focus == Focus::Search)
        curs_set(1);
    else if (focus == Focus::Editor)
        curs_set(editor.get_mode() == EditorMode::Insert ? 2 : 1);
    else
        curs_set(0);
}

void UI::resize()
{
    height = get_editor_dim().height;
    width = get_editor_dim().width;

    wbkgd(stdscr, COLOR_PAIR(Theme::Editor));
    erase();
    refresh();

    const int sb = effective_sidebar_width();

    if (height < 5 || width <= sb + line_number_width)
    {
        mvprintw(1, 0, "INVALID DIMENSIONS");
        refresh();
        return;
    }

    const bool cmd_open = command_line.is_active();
    const bool confirm_open = confirm_prompt.is_active();
    const bool prompt_open = input_prompt.is_active();
    const int bottom_rows = (cmd_open || confirm_open || prompt_open) ? 2 : 1;
    const int top_y = 1;

    int term_h = 0;
    if (terminal_visible)
    {
        term_h = std::clamp(terminal_height, 5, std::max(5, (height - bottom_rows - 3) / 2));
    }

    const int pane_height = height - top_y - bottom_rows - term_h;
    const int editor_y = top_y + 1;
    const int editor_height = pane_height - 1;
    const int editor_width = width - sb - line_number_width;
    const int editor_x = sb + line_number_width;
    const int editor_pane_width = width - sb;

    if (pane_height < 2 || editor_height < 1)
        return;

    header.resize(1, width, 0, 0);

    if (sidebar_visible)
    {
        if (side_view == SideView::Search)
        {
            search_panel.resize(pane_height, sb, top_y, 0);
            sidebar.resize(0, 0, 0, 0);
        }
        else
        {
            sidebar.resize(pane_height, sb, top_y, 0);
            search_panel.resize(0, 0, 0, 0);
        }
    }
    else
    {
        sidebar.resize(0, 0, 0, 0);
        search_panel.resize(0, 0, 0, 0);
    }

    tab_bar.resize(1, editor_pane_width, top_y, sb);
    line_number.resize(editor_height, line_number_width, editor_y, sb);
    editor.resize(editor_height, editor_width, editor_y, editor_x);
    messages_panel.resize(editor_height, editor_pane_width, editor_y, sb);

    const int picker_h = std::min(std::max(10, pane_height - 2), 22);
    const int picker_w = std::min(std::max(40, editor_pane_width - 4), 80);
    const int picker_y = top_y + std::max(1, (pane_height - picker_h) / 2);
    const int picker_x = sb + std::max(1, (editor_pane_width - picker_w) / 2);
    file_picker.resize(picker_h, picker_w, picker_y, picker_x);
    buffer_picker.resize(picker_h, picker_w, picker_y, picker_x);
    lsp_picker.resize(picker_h, picker_w, picker_y, picker_x);

    // Completion popup near the active cursor inside the editor pane.
    {
        const Cursor c = editor.get_cursor();
        const int sy = editor.get_scroll_y();
        const int items = completion_picker.is_active()
                              ? static_cast<int>(completion_picker.list().items.size())
                              : 8;
        const int comp_h = std::clamp(items + 2, 4, std::min(14, std::max(4, editor_height)));
        const int comp_w = std::min(48, std::max(24, editor_width - 2));
        int comp_y = editor_y + std::clamp(c.line - sy + 1, 0, std::max(0, editor_height - comp_h));
        int comp_x = editor_x + 2;
        if (comp_x + comp_w > sb + editor_pane_width)
            comp_x = std::max(sb, sb + editor_pane_width - comp_w);
        completion_picker.resize(comp_h, comp_w, comp_y, comp_x);
    }

    const int status_y = height - bottom_rows;
    if (cmd_open || confirm_open || prompt_open)
    {
        statusbar.resize(1, width, height - 2, 0);
        command_line.resize(1, width, height - 1, 0);
        confirm_prompt.resize(1, width, height - 1, 0);
        input_prompt.resize(1, width, height - 1, 0);
    }
    else
    {
        statusbar.resize(1, width, status_y, 0);
        command_line.resize(1, width, status_y, 0);
        confirm_prompt.resize(1, width, status_y, 0);
        input_prompt.resize(1, width, status_y, 0);
    }

    if (terminal_visible && term_h > 0)
    {
        terminal.resize(term_h, width, status_y - term_h, 0);
        terminal.set_visible(true);
        terminal.on_resized();
    }
    else
    {
        terminal.resize(0, 0, 0, 0);
        terminal.set_visible(false);
    }

    refresh();
}

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

void UI::execute_command()
{
    const char prompt = command_line.prompt();
    const std::string line = StrUtils::trim(command_line.get_input());
    command_line.close();
    focus = Focus::Editor;
    editor.enter_normal_mode();
    resize();

    if (prompt == '/' || prompt == '?')
    {
        if (line.empty())
        {
            Messages::info("Empty pattern");
            return;
        }
        const SearchDirection dir =
            (prompt == '?') ? SearchDirection::Backward : SearchDirection::Forward;
        const std::string err = core.search_start(line, dir);
        if (!err.empty())
            Messages::info(err);
        else
            Messages::info(std::format(
                "{} matches",
                core.search().matches().size()));
        return;
    }

    if (line.empty())
        return;

    ExCommands::instance().execute(*this, line);
}

void UI::ex_quit(bool bang)
{
    if (core.buffers().size() > 1)
    {
        close_active_tab(bang);
        return;
    }

    if (!bang && core.buffers().active().buffer().is_dirty())
    {
        open_save_confirm(ConfirmIntent::Quit);
        return;
    }
    request_quit();
}

void UI::ex_write(bool bang, const std::string &path)
{
    (void)bang;
    if (!path.empty())
    {
        if (!editor.get_buffer().save_as(path))
        {
            Messages::error("E212: Can't open file for writing");
            return;
        }
        statusbar.set_filename(core.buffers().active().display_name());
        Messages::info(std::format("\"{}\" written", core.buffers().active().display_name()));
        clear_recovery_for_buffer(editor.get_buffer());
        core.scm().request_refresh();
        core.lsp().notify_save(editor.get_buffer());
        core.notify_buffer_saved(editor.get_buffer());
        return;
    }

    if (editor.get_buffer().get_buffer_path().empty())
    {
        Messages::error("E32: No file name");
        return;
    }

    if (editor.get_buffer().has_load_error())
    {
        Messages::error(std::format(
            "E13: Cannot save — file was not loaded ({})",
            editor.get_buffer().get_load_error()));
        return;
    }

    if (editor.get_buffer().save())
    {
        statusbar.set_filename(core.buffers().active().display_name());
        Messages::info(std::format("\"{}\" written", core.buffers().active().display_name()));
        clear_recovery_for_buffer(editor.get_buffer());
        core.scm().request_refresh();
        core.lsp().notify_save(editor.get_buffer());
        core.notify_buffer_saved(editor.get_buffer());
    }
    else
    {
        Messages::error("E212: Can't open file for writing");
    }
}

void UI::ex_write_quit(bool bang, const std::string &path)
{
    if (!path.empty())
    {
        if (!editor.get_buffer().save_as(path))
        {
            Messages::error("E212: Can't open file for writing");
            return;
        }
    }
    else if (editor.get_buffer().get_buffer_path().empty())
    {
        if (!bang)
        {
            Messages::error("E32: No file name");
            return;
        }
    }
    else if (editor.get_buffer().has_load_error())
    {
        Messages::error(std::format(
            "E13: Cannot save — file was not loaded ({})",
            editor.get_buffer().get_load_error()));
        return;
    }
    else if (!editor.get_buffer().save())
    {
        Messages::error("E212: Can't open file for writing");
        return;
    }

    clear_recovery_for_buffer(editor.get_buffer());
    if (core.buffers().size() > 1)
        close_active_tab(true);
    else
        request_quit();
}

void UI::ex_edit(bool bang, const std::string &path)
{
    if (path.empty())
    {
        if (!bang && editor.get_buffer().is_dirty())
        {
            Messages::error("E37: No write since last change (add ! to override)");
            return;
        }
        editor.get_buffer().load();
        editor.set_cursor_position(0, 0);
        if (editor.get_buffer().has_load_error())
        {
            Messages::error(std::format(
                "Failed to reload {}: {}",
                editor.get_buffer().get_buffer_path().string(),
                editor.get_buffer().get_load_error()));
        }
        else
        {
            Messages::info("Buffer reloaded");
        }
        return;
    }

    if (!bang && editor.get_buffer().is_dirty() && core.buffers().size() == 1)
    {
        // Opening another file in a new tab is fine; only warn when replacing sole dirty buf.
    }

    core.buffers().open_file(path);
    note_opened_file(path);
    sync_active_tab();
    editor.enter_normal_mode();
    refresh_scm(fs::path(path));
    focus = Focus::Editor;
    Messages::info(std::format("\"{}\"", core.buffers().active().display_name()));
}

void UI::ex_bnext()
{
    core.buffers().next_tab();
    sync_active_tab();
}

void UI::ex_bprevious()
{
    core.buffers().prev_tab();
    sync_active_tab();
}

void UI::ex_bdelete(bool bang)
{
    close_active_tab(bang);
}

void UI::ex_messages()
{
    messages_panel.open("Messages");
    focus = Focus::Messages;
}

void UI::ex_logs(const std::string &which)
{
    struct Entry
    {
        const char *key;
        const char *path;
        const char *title;
    };
    static constexpr Entry kLogs[] = {
        {"noni", "logs/noni.log", "noni.log (app Logger)"},
        {"lsp", "logs/lsp.stderr.log", "lsp.stderr.log (clangd/pylsp stderr)"},
        {"install", "logs/lsp-install.log", "lsp-install.log"},
        {"grammar", "logs/grammar-install.log", "grammar-install.log"},
    };

    const std::string w = StrUtils::to_lower(StrUtils::trim(which));
    std::vector<std::string> out;
    out.push_back("LOGS — file-backed (not the statusbar Messages ring)");
    out.push_back("  :logs          → all (last ~80 lines each)");
    out.push_back("  :logs noni|lsp|install|grammar");
    out.push_back("  Tip: :messages only shows Messages::info/warning, not Logger.");
    out.push_back("");

    auto append_file = [&](const Entry &e) {
        out.push_back(std::format("═══ {} ({}) ═══", e.title, e.path));
        std::ifstream in(e.path);
        if (!in)
        {
            out.push_back(std::format("  (missing — nothing written yet)"));
            out.push_back("");
            return;
        }
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line))
            lines.push_back(std::move(line));
        const std::size_t keep = 80;
        const std::size_t start =
            lines.size() > keep ? lines.size() - keep : 0;
        for (std::size_t i = start; i < lines.size(); ++i)
            out.push_back(lines[i]);
        out.push_back("");
    };

    bool any = false;
    for (const auto &e : kLogs)
    {
        if (!w.empty() && w != "all" && w != e.key)
            continue;
        append_file(e);
        any = true;
    }
    if (!any)
    {
        Messages::error("E149: Unknown log — use noni|lsp|install|grammar");
        return;
    }

    Messages::set_lines(std::move(out));
    messages_panel.open(w.empty() || w == "all" ? "Logs" : std::format("Logs: {}", w));
    focus = Focus::Messages;
    keys.clear_chord();
}

void UI::ex_help(const std::string &topic)
{
    ExCommands::instance().ensure_registered();
    std::vector<std::string> lines;
    std::string err;
    if (!HelpDocs::build(topic, config, ExCommands::instance().all(), lines, err))
    {
        Messages::error(err.empty() ? "E149: No help for topic" : err);
        return;
    }
    Messages::set_lines(std::move(lines));
    messages_panel.open(topic.empty() || topic == "index" ? "Help" : std::format("Help: {}", topic));
    focus = Focus::Messages;
    keys.clear_chord();
}

void UI::ex_sidebar(const std::string &arg)
{
    const std::string a = StrUtils::to_lower(StrUtils::trim(arg));
    if (a.empty() || a == "toggle")
        toggle_sidebar();
    else if (a == "open" || a == "show" || a == "on")
        open_sidebar(true);
    else if (a == "close" || a == "hide" || a == "off")
        close_sidebar();
    else
        Messages::error("Usage: :sidebar [open|close|toggle]");
}

void UI::ex_find()
{
    open_file_search();
}

void UI::ex_buffers()
{
    open_buffer_search();
}

void UI::ex_workspace(const std::string &arg)
{
    const std::string a = StrUtils::trim(arg);
    if (a.empty())
    {
        if (core.workspace().has_root())
            Messages::info(std::format("Workspace: {}", core.workspace().root().string()));
        else
            Messages::info("No workspace root");
        return;
    }
    apply_workspace_root(fs::path(a), true);
}

void UI::ex_search()
{
    open_search_view(true);
}

void UI::ex_undo()
{
    if (!core.buffers().has_tabs())
        return;
    if (!editor.undo())
        Messages::info("Nothing to undo");
}

void UI::ex_redo()
{
    if (!core.buffers().has_tabs())
        return;
    if (!editor.redo())
        Messages::info("Nothing to redo");
}

void UI::toggle_terminal()
{
    if (!terminal_visible)
    {
        open_terminal(true);
        return;
    }

    if (focus == Focus::Terminal)
    {
        close_terminal();
        return;
    }

    open_terminal(true);
}

void UI::open_terminal(bool focus_terminal)
{
    terminal_visible = true;
    terminal.set_visible(true);
    auto tcfg = terminal.session().config();
    tcfg.cwd = project_root().string();
    if (tcfg.shell.empty())
        tcfg.shell = config.terminal.shell;
    tcfg.scrollback = config.terminal.scrollback;
    terminal.apply_config(tcfg);
    terminal.set_cwd(project_root().string());
    keys.clear_chord();
    editor.enter_normal_mode();

    if (focus_terminal)
    {
        focus = Focus::Terminal;
        terminal.set_focused(true);
    }

    resize();
    terminal.ensure_started();
    terminal.on_resized();
}

void UI::close_terminal()
{
    terminal_visible = false;
    terminal.set_visible(false);
    terminal.set_focused(false);
    if (focus == Focus::Terminal)
        focus = Focus::Editor;
    keys.clear_chord();
    resize();
}

void UI::ex_terminal(const std::string &arg)
{
    const std::string a = StrUtils::to_lower(StrUtils::trim(arg));
    if (a.empty() || a == "open" || a == "toggle")
    {
        if (a == "toggle")
            toggle_terminal();
        else
            open_terminal(true);
        return;
    }
    if (a == "close" || a == "hide")
    {
        close_terminal();
        return;
    }
    if (a == "kill")
    {
        terminal.stop();
        Messages::info("Terminal process killed");
        return;
    }
    if (a == "clear")
    {
        if (!terminal_visible)
            open_terminal(false);
        terminal.clear_screen();
        return;
    }
    Messages::error("Usage: :terminal [open|close|toggle|kill|clear]");
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

void UI::handle_inputs()
{
    const int ch = getch();

    if (ch == ERR)
        return;

    if (ch == KEY_RESIZE)
    {
        resize();
        return;
    }

    auto dispatch_resolved = [this](int key, InputContext ctx) -> bool {
        const ResolveResult result = keys.resolve(key, when_context(), ctx);
        if (result.status == ResolveStatus::Matched)
        {
            if (!commands.execute(result.command_id))
                Logger::warning(std::format("unhandled command: {}", result.command_id));
            return true;
        }
        return result.status == ResolveStatus::Prefix;
    };

    if (focus == Focus::Command)
    {
        if (ch == 27 || ch == 3)
        {
            (void)dispatch_resolved(ch, InputContext::CommandLine);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER)
        {
            execute_command();
            return;
        }
        command_line.handle_input(ch);
        if (!command_line.is_active())
            return_to_normal();
        return;
    }

    if (focus == Focus::Messages)
    {
        if (ch == 27 || ch == 3)
        {
            (void)dispatch_resolved(ch, InputContext::Messages);
            return;
        }
        messages_panel.handle_input(ch);
        if (!messages_panel.is_active())
            return_to_normal();
        return;
    }

    if (focus == Focus::FileSearch)
    {
        if (ch == 27 || ch == 3)
        {
            close_file_search(false);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER)
        {
            close_file_search(true);
            return;
        }
        file_picker.handle_input(ch);
        return;
    }

    if (focus == Focus::BufferSearch)
    {
        if (ch == 27 || ch == 3)
        {
            close_buffer_search(false);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER)
        {
            close_buffer_search(true);
            return;
        }
        buffer_picker.handle_input(ch);
        return;
    }

    if (focus == Focus::LspPicker)
    {
        if (ch == 27 || ch == 3)
        {
            close_lsp_picker(false);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER)
        {
            close_lsp_picker(true);
            return;
        }
        lsp_picker.handle_input(ch);
        return;
    }

    if (focus == Focus::Completion)
    {
        if (ch == 27 || ch == 3)
        {
            close_completion(false);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER || ch == '\t')
        {
            close_completion(true);
            return;
        }
        completion_picker.handle_input(ch);
        return;
    }

    if (focus == Focus::Terminal)
    {
        if (ch == 27 || ch == 29)
        {
            return_to_normal();
            return;
        }
        // Allow F4 / explicit control bindings; never Space leader / editor chords.
        if (dispatch_resolved(ch, InputContext::Terminal))
            return;
        terminal.handle_input(ch);
        return;
    }

    if (focus == Focus::Search)
    {
        // Leader chords + ctrl bindings; typing falls through to the panel.
        if (dispatch_resolved(ch, InputContext::SearchResults))
            return;

        const SearchPanelAction action = search_panel.handle_input(ch);
        if (action == SearchPanelAction::FocusEditor)
        {
            return_to_normal();
            return;
        }
        if (action == SearchPanelAction::OpenMatch)
        {
            const TextMatch m = search_panel.selected_match();
            if (!m.path.empty())
            {
                core.buffers().open_file(m.path);
                sync_active_tab();
                editor.set_cursor_position(m.line, m.column);
                editor.enter_normal_mode();
                focus = Focus::Editor;
                keys.clear_chord();
            }
            return;
        }
        if (action == SearchPanelAction::ReplaceAll)
        {
            apply_replace_all();
            return;
        }
        return;
    }

    if (focus == Focus::Confirm)
    {
        const ConfirmChoice choice = confirm_prompt.handle_input(ch);
        if (choice != ConfirmChoice::Pending)
            resolve_save_confirm(choice);
        return;
    }

    if (focus == Focus::Prompt)
    {
        if (ch == '\n' || ch == KEY_ENTER)
        {
            resolve_sidebar_prompt();
            return;
        }
        if (ch == 27 || ch == 3)
        {
            (void)dispatch_resolved(ch, InputContext::PromptInput);
            if (input_prompt.is_active())
            {
                input_prompt.handle_input(ch);
            }
            if (!input_prompt.is_active())
            {
                prompt_intent = PromptIntent::None;
                focus = Focus::Sidebar;
                resize();
                Messages::info("Cancelled");
            }
            return;
        }
        input_prompt.handle_input(ch);
        if (!input_prompt.is_active())
        {
            prompt_intent = PromptIntent::None;
            focus = Focus::Sidebar;
            resize();
            Messages::info("Cancelled");
        }
        return;
    }

    if (focus == Focus::Sidebar)
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
        return;
    }

    // Editor focus (and default): resolve commands, else modal editor input.
    {
        const InputContext ctx = input_context_for(focus, editor.get_mode(), false);
        if (dispatch_resolved(ch, ctx))
            return;
        if (focus == Focus::Editor)
            editor.handle_input(ch);
    }
}

Dimentions UI::get_editor_dim() const
{
    Dimentions d;
    getmaxyx(stdscr, d.height, d.width);
    return d;
}
