#include <ui/ui.hpp>
#include <utils/async.hpp>
#include <ui/theme.hpp>
#include <editor/ex_commands.hpp>
#include <editor/buffer_search.hpp>
#include <commands/command.hpp>
#include <scm/scm_git.hpp>
#include <lsp/lsp_service.hpp>
#include <syntax/grammar_installer.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <utils/str.hpp>
#include <utils/text_metrics.hpp>
#include <algorithm>
#include <cstdint>
#include <csignal>
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
    const fs::path workspace = find_workspace_root(hint);

    if (file_path.empty())
    {
        Logger::info("No file path provided");
        core.buffers().open_untitled();
    }
    else
    {
        Logger::info(std::format("Opening file: {}", file_path.string()));
        core.buffers().open_file(file_path);
    }

    // Always root the explorer at the project (nearest folder with .git).
    sidebar.set_project_path(workspace);
    search_panel.set_root(workspace);
    refresh_scm(workspace);
    file_picker.warm(workspace);

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
}

UI::~UI()
{
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
    config = AppConfig::load("config.json");
    keys.load(config);
    GrammarInstaller::set_auto_install(config.syntax_auto_install);

    std::vector<LspServerConfig> servers;
    servers.reserve(config.lsp_servers.size());
    for (const auto &s : config.lsp_servers)
    {
        LspServerConfig c;
        c.language = s.language;
        c.command = s.command;
        c.root_markers = s.root_markers;
        servers.push_back(std::move(c));
    }
    core.lsp().set_server_configs(std::move(servers));

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

    commands.register_command(Commands::LspShowStatus, [this]() {
        core.lsp().pump();
        Messages::info(core.lsp().status_summary());
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

    // Binding diagnostics
    std::vector<CommandId> unbound;
    std::vector<CommandId> unknown;
    commands.diagnose(keys.bound_commands(), &unbound, &unknown);
    for (const auto &id : unknown)
        Logger::warning(std::format("keybinding references unknown command: {}", id));
    for (const auto &id : unbound)
        Logger::debug(std::format("command registered but unbound: {}", id));
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
    const fs::path root = find_workspace_root(hint);
    core.scm().set_workspace_root(root);
    // set_workspace_root already requests refresh when root changes; force when same.
    core.scm().request_refresh();
    scm_gen_seen_ = 0; // force header/badge update when snapshot arrives
    core.lsp().set_workspace_root(root);
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

fs::path UI::find_workspace_root(const fs::path &hint) const
{
    const fs::path git_root = ScmGit::find_repository_root(hint);
    if (!git_root.empty())
        return git_root;

    std::error_code ec;
    fs::path cur = hint.empty() ? fs::current_path() : hint;
    if (fs::is_regular_file(cur, ec))
        cur = cur.parent_path();
    cur = fs::weakly_canonical(cur, ec);
    if (ec)
        cur = hint.empty() ? fs::current_path() : hint;

    auto is_project_root = [&](const fs::path &dir) -> bool {
        static const char *markers[] = {
            // VCS / noni
            ".git", ".hg", ".svn", ".noni",
            // C / C++
            "CMakeLists.txt", "Makefile", "meson.build", "configure.ac",
            "compile_commands.json", "vcpkg.json", "conanfile.txt", "conanfile.py",
            // Rust / Go / Zig / Swift
            "Cargo.toml", "go.mod", "build.zig", "Package.swift",
            // Python
            "pyproject.toml", "setup.py", "setup.cfg", "requirements.txt",
            "Pipfile", "poetry.lock", "tox.ini",
            // Java / JVM
            "pom.xml", "build.gradle", "build.gradle.kts",
            "settings.gradle", "settings.gradle.kts", "build.sbt",
            "project.clj", "deps.edn",
            // JS / TS
            "package.json", "pnpm-workspace.yaml", "lerna.json",
            // Lua
            ".luarc.json", "selene.toml",
            // PHP / Ruby / Elixir / Dart
            "composer.json", "Gemfile", "mix.exs", "pubspec.yaml",
        };
        for (const char *m : markers)
        {
            if (fs::exists(dir / m, ec))
                return true;
        }

        // .NET solutions/projects and Lua rockspecs (extension-based)
        for (const auto &entry : fs::directory_iterator(dir, ec))
        {
            if (ec || !entry.is_regular_file(ec))
                continue;
            const std::string ext = entry.path().extension().string();
            if (ext == ".sln" || ext == ".csproj" || ext == ".fsproj" ||
                ext == ".vbproj" || ext == ".rockspec")
                return true;
        }
        return false;
    };

    fs::path walk = cur;
    while (!walk.empty())
    {
        if (is_project_root(walk))
            return walk;
        const fs::path parent = walk.parent_path();
        if (parent == walk)
            break;
        walk = parent;
    }

    // No markers: use the opened file's directory (or cwd).
    return cur.empty() ? fs::current_path() : cur;
}

void UI::open_file_search()
{
    keys.clear_chord();
    if (command_line.is_active())
        command_line.close();
    if (messages_panel.is_active())
        messages_panel.close();
    if (confirm_prompt.is_active())
    {
        confirm_prompt.close();
        confirm_intent = ConfirmIntent::None;
    }

    editor.enter_normal_mode();
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
    sync_active_tab();
    editor.enter_normal_mode();
    refresh_scm(path);
    Messages::info(std::format("\"{}\"", core.buffers().active().display_name()));
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
    if (id <= 0)
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
        sidebar.reveal_path(path);
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

    if (completion_picker.is_active())
        completion_picker.close();
    core.lsp().cancel_completion();

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
    const bool was_last = core.buffers().size() == 1;
    core.buffers().close_active(true);
    core.on_buffer_closed(closed);

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
    core.scm().request_refresh();
    core.lsp().notify_save(editor.get_buffer());
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
            if (!before.empty() && !after.empty() && core.buffers().has_tabs() &&
                core.buffers().active().buffer().get_buffer_path() == before)
            {
                core.buffers().active().buffer().set_buffer_path(after);
                sync_active_tab();
            }
            file_picker.reindex();
        }
        else
            Messages::error(std::format("Could not rename to {}", name));
        break;
    }
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

    // Close tab if this file is open.
    if (core.buffers().has_tabs() && core.buffers().active().buffer().get_buffer_path() == path)
        close_active_tab(true);

    if (!sidebar.delete_path(path))
    {
        Messages::error(std::format("Could not delete {}", path.filename().string()));
        return;
    }

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
        focus == Focus::Completion || focus == Focus::Confirm || focus == Focus::Prompt ||
        focus == Focus::Terminal || focus == Focus::Search)
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
    core.lsp().pump();
    poll_completion_result();

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

    line_number.sync(
        editor.get_scroll_y(),
        c.line,
        static_cast<int>(editor.get_buffer().lines().size()),
        &editor.get_buffer().diagnostics());

    wbkgd(stdscr, COLOR_PAIR(Theme::Editor));
    werase(stdscr);
    wnoutrefresh(stdscr);

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

    if (messages_panel.is_active())
        wnoutrefresh(messages_panel.get_window());

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
    else if (completion_picker.is_active())
    {
        wnoutrefresh(editor.get_window());
        wnoutrefresh(completion_picker.get_window());
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
    while (running)
    {
        file_picker.poll();
        search_panel.poll();
        terminal.poll();
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
        core.scm().request_refresh();
        core.lsp().notify_save(editor.get_buffer());
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
        core.scm().request_refresh();
        core.lsp().notify_save(editor.get_buffer());
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
    messages_panel.open();
    focus = Focus::Messages;
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
