#include <ui/ui.hpp>
#include <utils/async.hpp>
#include <ui/theme.hpp>
#include <editor/ex_commands.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <utils/str.hpp>
#include <algorithm>
#include <csignal>
#include <fstream>
#include <locale>
#include <format>
#include <regex>
#include <termios.h>
#include <unistd.h>
#include <utils/fs.hpp>

UI::UI(const fs::path file_path = "")
{
    Background::instance().start(2);

    const fs::path hint = file_path.empty() ? fs::current_path() : file_path;
    const fs::path workspace = find_workspace_root(hint);

    if (file_path.empty())
    {
        Logger::info("No file path provided");
        buffers.open_untitled();
    }
    else
    {
        Logger::info(std::format("Opening file: {}", file_path.string()));
        buffers.open_file(file_path);
    }

    // Always root the explorer at the project (nearest folder with .git).
    sidebar.set_project_path(workspace);
    search_panel.set_root(workspace);
    header.refresh_git(workspace);
    file_picker.warm(workspace);

    tab_bar.set_manager(&buffers);
    sync_active_tab();
    load_config();
    init();
    register_actions();
}

UI::~UI()
{
    terminal.stop();
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
}

void UI::register_actions()
{
    keys.register_command("noni.mode.normal", [this]() { return_to_normal(); });

    keys.register_command("workbench.action.files.save", [this]() {
        if (!buffers.has_tabs())
            return;
        if (editor.get_buffer().save())
            Messages::info(std::format("\"{}\" written", buffers.active().display_name()));
        else
            Messages::error("E212: Can't open file for writing");
    });

    keys.register_command("workbench.action.closeActiveEditor", [this]() {
        close_active_tab(false);
    });

    keys.register_command("workbench.action.nextEditor", [this]() {
        buffers.next_tab();
        sync_active_tab();
    });

    keys.register_command("workbench.action.previousEditor", [this]() {
        buffers.prev_tab();
        sync_active_tab();
    });

    keys.register_command("noni.focus.toggleSidebar", [this]() {
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

    keys.register_command("noni.focus.sidebar", [this]() {
        open_sidebar(true);
    });

    keys.register_command("noni.focus.editor", [this]() {
        return_to_normal();
    });

    keys.register_command("workbench.action.toggleSidebarVisibility", [this]() {
        toggle_sidebar();
    });

    keys.register_command("workbench.action.closeSidebar", [this]() {
        close_sidebar();
    });

    keys.register_command("workbench.view.explorer", [this]() {
        open_explorer_view(true);
    });

    keys.register_command("workbench.view.search", [this]() {
        open_search_view(true);
    });

    keys.register_command("workbench.action.findInFiles", [this]() {
        open_search_view(true);
    });

    keys.register_command("noni.command.open", [this]() {
        command_line.open();
        focus = Focus::Command;
        keys.clear_chord();
        resize();
    });

    keys.register_command("workbench.action.quit", [this]() {
        ex_quit(false);
    });

    keys.register_command("workbench.action.quickOpen", [this]() {
        open_file_search();
    });

    keys.register_command("noni.search.files", [this]() {
        open_file_search();
    });

    keys.register_command("workbench.action.terminal.toggle", [this]() {
        toggle_terminal();
    });

    keys.register_command("workbench.action.terminal.focus", [this]() {
        open_terminal(true);
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
    std::sort(matches.begin(), matches.end(), [](const TextMatch &a, const TextMatch &b) {
        if (a.path != b.path)
            return a.path < b.path;
        if (a.line != b.line)
            return a.line > b.line;
        return a.column > b.column;
    });

    int files_touched = 0;
    int replacements = 0;
    fs::path current;
    std::vector<std::string> lines;
    bool dirty_file = false;

    auto write_current = [&]() {
        if (current.empty() || !dirty_file)
            return;
        FS fs;
        if (!fs.write_file(current, lines))
            return;
        ++files_touched;
        if (buffers.has_tabs() && buffers.active().buffer.get_buffer_path() == current)
        {
            buffers.active().buffer.load();
            sync_active_tab();
        }
        dirty_file = false;
    };

    for (const auto &m : matches)
    {
        if (m.path != current)
        {
            write_current();
            current = m.path;
            lines.clear();
            dirty_file = false;
            std::ifstream in(current);
            if (!in)
            {
                current.clear();
                continue;
            }
            std::string line;
            while (std::getline(in, line))
            {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                lines.push_back(line);
            }
        }

        if (current.empty() || m.line < 0 || m.line >= static_cast<int>(lines.size()))
            continue;

        std::string &line = lines[static_cast<std::size_t>(m.line)];
        if (opts.use_regex)
        {
            try
            {
                auto flags = std::regex::ECMAScript;
                if (!opts.match_case)
                    flags |= std::regex::icase;
                std::regex re(q, flags);
                const std::string before = line;
                line = std::regex_replace(line, re, r);
                if (line != before)
                {
                    ++replacements;
                    dirty_file = true;
                }
            }
            catch (...)
            {
            }
        }
        else
        {
            if (m.column < 0 ||
                m.column + static_cast<int>(q.size()) > static_cast<int>(line.size()))
                continue;
            std::string span = line.substr(static_cast<std::size_t>(m.column), q.size());
            const bool ok = opts.match_case
                ? (span == q)
                : (StrUtils::to_lower(span) == StrUtils::to_lower(q));
            if (!ok)
                continue;
            line.replace(static_cast<std::size_t>(m.column), q.size(), r);
            ++replacements;
            dirty_file = true;
        }
    }
    write_current();

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
    if (buffers.has_tabs())
    {
        const auto path = buffers.active().buffer.get_buffer_path();
        if (!path.empty())
            return path.parent_path().empty() ? fs::current_path() : path.parent_path();
    }
    return fs::current_path();
}

fs::path UI::find_workspace_root(const fs::path &hint) const
{
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

    buffers.open_file(path);
    sync_active_tab();
    editor.enter_normal_mode();
    header.refresh_git(path.parent_path());
    Messages::info(std::format("\"{}\"", buffers.active().display_name()));
}

void UI::sync_active_tab()
{
    if (!buffers.has_tabs())
        buffers.open_untitled();

    editor.bind(&buffers.active());
    statusbar.set_filename(buffers.active().display_name());

    const fs::path path = buffers.active().buffer.get_buffer_path();
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
    if (!buffers.has_tabs())
        return true;

    if (!force && buffers.active().buffer.is_dirty())
    {
        open_save_confirm(ConfirmIntent::CloseTab);
        return false;
    }

    const bool was_last = buffers.size() == 1;
    buffers.close_active(true);

    if (was_last)
    {
        if (running)
            buffers.open_untitled();
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
    const std::string name = buffers.active().display_name();
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

    if (!editor.get_buffer().save())
    {
        Messages::error("E212: Can't open file for writing");
        return false;
    }

    statusbar.set_filename(buffers.active().display_name());
    Messages::info(std::format("\"{}\" written", buffers.active().display_name()));
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
        if (buffers.size() > 1)
        {
            close_active_tab(force);
            return;
        }
        if (!force && buffers.active().buffer.is_dirty())
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
                buffers.open_file(path);
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
            if (!before.empty() && !after.empty() && buffers.has_tabs() &&
                buffers.active().buffer.get_buffer_path() == before)
            {
                buffers.active().buffer.set_buffer_path(after);
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
    if (buffers.has_tabs() && buffers.active().buffer.get_buffer_path() == path)
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
        focus == Focus::Confirm || focus == Focus::Prompt ||
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
    if (!buffers.has_tabs())
        return;

    Cursor c = editor.get_cursor();
    statusbar.set_cursor_position(c.line + 1, c.column + 1);
    update_statusbar_mode();
    update_cursor_visibility();

    line_number.sync(
        editor.get_scroll_y(),
        c.line,
        static_cast<int>(editor.get_buffer().lines().size()));

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
        if (sidebar.poll() && buffers.has_tabs())
        {
            const fs::path path = buffers.active().buffer.get_buffer_path();
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
    const std::string line = StrUtils::trim(command_line.get_input());
    command_line.close();
    focus = Focus::Editor;
    editor.enter_normal_mode();
    resize();

    if (line.empty())
        return;

    ExCommands::instance().execute(*this, line);
}

void UI::ex_quit(bool bang)
{
    if (buffers.size() > 1)
    {
        close_active_tab(bang);
        return;
    }

    if (!bang && buffers.active().buffer.is_dirty())
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
        statusbar.set_filename(buffers.active().display_name());
        Messages::info(std::format("\"{}\" written", buffers.active().display_name()));
        return;
    }

    if (editor.get_buffer().get_buffer_path().empty())
    {
        Messages::error("E32: No file name");
        return;
    }

    if (editor.get_buffer().save())
    {
        statusbar.set_filename(buffers.active().display_name());
        Messages::info(std::format("\"{}\" written", buffers.active().display_name()));
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
    else if (!editor.get_buffer().save())
    {
        Messages::error("E212: Can't open file for writing");
        return;
    }

    if (buffers.size() > 1)
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
        Messages::info("Buffer reloaded");
        return;
    }

    if (!bang && editor.get_buffer().is_dirty() && buffers.size() == 1)
    {
        // Opening another file in a new tab is fine; only warn when replacing sole dirty buf.
    }

    buffers.open_file(path);
    sync_active_tab();
    editor.enter_normal_mode();
    header.refresh_git(fs::path(path).parent_path());
    focus = Focus::Editor;
    Messages::info(std::format("\"{}\"", buffers.active().display_name()));
}

void UI::ex_bnext()
{
    buffers.next_tab();
    sync_active_tab();
}

void UI::ex_bprevious()
{
    buffers.prev_tab();
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
    Messages::error("Usage: :terminal [open|close|toggle]");
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
    return buffers;
}

void UI::handle_inputs()
{
    const int ch = getch();

    if (ch == ERR)
        return; // idle tick for background jobs

    if (ch == KEY_RESIZE)
    {
        resize();
        return;
    }

    // Typing in command / messages stays raw (except Esc via keybindings).
    if (focus == Focus::Command)
    {
        if (ch == 27 || ch == 3)
        {
            keys.handle(ch, when_context());
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
            keys.handle(ch, when_context());
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

    if (focus == Focus::Terminal)
    {
        // Esc / Ctrl+] leave terminal focus (shell keeps running).
        if (ch == 27 || ch == 29)
        {
            return_to_normal();
            return;
        }
        if (ch == KEY_F(4))
        {
            toggle_terminal();
            return;
        }
        // Keys go to the shell (Ctrl+C included). Don't steal Space for chords.
        terminal.handle_input(ch);
        return;
    }

    if (focus == Focus::Search)
    {
        // Ctrl chords / F4 still work; Tab and typing stay in the panel.
        if ((ch >= 1 && ch <= 26 && ch != 3) || ch == KEY_F(4))
        {
            if (keys.handle(ch, when_context()))
                return;
        }

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
                buffers.open_file(m.path);
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

        // Global shortcuts still work (Ctrl+B, Ctrl+P, ...).
        if (keys.handle(ch, when_context()))
            return;

        const SidebarAction action = sidebar.handle_input(ch);
        if (action == SidebarAction::OpenFile)
        {
            const fs::path path = sidebar.get_selected_path();
            if (!path.empty() && fs::is_regular_file(path))
            {
                buffers.open_file(path);
                sync_active_tab();
                editor.enter_normal_mode();
                header.refresh_git(find_workspace_root(path));
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
                buffers.has_tabs() && !from.empty() && !to.empty() &&
                buffers.active().buffer.get_buffer_path() == from)
            {
                buffers.active().buffer.set_buffer_path(to);
                sync_active_tab();
            }
            file_picker.reindex();
            return;
        }
        return;
    }

    // VS Code-style JSON keybindings first.
    if (keys.handle(ch, when_context()))
        return;

    // Fallback: editor local input (vim motions, typing).
    if (focus == Focus::Editor)
        editor.handle_input(ch);
}

Dimentions UI::get_editor_dim() const
{
    Dimentions d;
    getmaxyx(stdscr, d.height, d.width);
    return d;
}
