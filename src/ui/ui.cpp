#include <ui/ui.hpp>
#include <ui/theme.hpp>
#include <utils/logger.hpp>
#include <locale>
#include <format>

UI::UI(const fs::path file_path = "")
{
    if (file_path.empty())
    {
        Logger::info("No file path provided");
    }
    else
    {
        std::string filename = file_path.filename().string();
        std::string parent_folder = file_path.parent_path().string();
        Logger::info(std::format("Opening file: {}", file_path.string()));
        statusbar.set_filename(filename);
        sidebar.set_project_path(parent_folder);
        editor.buffer.set_buffer_path(file_path);
        header.refresh_git(parent_folder);
    }

    init();
}

UI::~UI()
{
    endwin();
    Logger::debug("UI destructor called");
}

void UI::init()
{
    initscr();

    setlocale(LC_ALL, "");
    cbreak();
    noecho();

    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors())
        Theme::init();

    resize();
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
    case Focus::Editor:
        statusbar.set_mode(editor.get_mode_label());
        break;
    }
}

void UI::update_cursor_visibility()
{
    if (focus == Focus::Command)
        curs_set(1);
    else if (focus == Focus::Editor && editor.get_mode() == EditorMode::Insert)
        curs_set(1);
    else
        curs_set(0);
}

void UI::resize()
{
    height = get_editor_dim().height;
    width = get_editor_dim().width;

    refresh();

    if (height < 4 || width <= sidebar_width + line_number_width)
    {
        mvprintw(1, 0, "INVALID DIMENSIONS");
        refresh();
        return;
    }

    const int content_height = height - 3;
    const int editor_width = width - sidebar_width - line_number_width;
    const int editor_x = sidebar_width + line_number_width;

    sidebar.resize(content_height, sidebar_width, 1, 0);
    line_number.resize(content_height, line_number_width, 1, sidebar_width);
    editor.resize(content_height, editor_width, 1, editor_x);
    messages_panel.resize(content_height, line_number_width + editor_width, 1, sidebar_width);
    statusbar.resize(1, width, height - 2, 0);
    command_line.resize(1, width, height - 1, 0);
    header.resize(1, width, 0, 0);

    refresh();
}

void UI::render()
{
    Cursor c = editor.get_cursor();
    statusbar.set_cursor_position(c.line + 1, c.column + 1);
    update_statusbar_mode();
    update_cursor_visibility();

    line_number.sync(
        editor.get_scroll_y(),
        c.line,
        static_cast<int>(editor.buffer.lines().size()));

    header.draw();
    sidebar.draw();
    line_number.draw();
    editor.draw();

    if (messages_panel.is_active())
        messages_panel.draw();

    statusbar.draw();

    if (command_line.is_active())
        command_line.draw();

    wnoutrefresh(header.get_window());
    wnoutrefresh(sidebar.get_window());
    wnoutrefresh(line_number.get_window());
    wnoutrefresh(editor.get_window());

    if (messages_panel.is_active())
        wnoutrefresh(messages_panel.get_window());

    wnoutrefresh(statusbar.get_window());

    if (command_line.is_active())
        wnoutrefresh(command_line.get_window());

    doupdate();
}

void UI::run()
{
    Logger::info("UI main loop started");
    while (running)
    {
        render();
        handle_inputs();
    }
    Logger::info("UI main loop stopped");
}

void UI::request_quit()
{
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

void UI::handle_inputs()
{
    const int ch = getch();

    if (ch == KEY_RESIZE)
    {
        resize();
        return;
    }

    if (focus == Focus::Command)
    {
        if (ch == '\n' || ch == KEY_ENTER)
        {
            focus = Focus::Editor;
            editor.enter_normal_mode();
            command_line.close();
            return;
        }

        command_line.handle_input(ch);
        if (!command_line.is_active())
        {
            focus = Focus::Editor;
            editor.enter_normal_mode();
        }
        return;
    }

    if (focus == Focus::Messages)
    {
        messages_panel.handle_input(ch);
        if (!messages_panel.is_active())
            focus = Focus::Editor;
        return;
    }

    switch (ch)
    {
    case ':':
        if (focus == Focus::Editor && editor.get_mode() == EditorMode::Normal)
        {
            command_line.open();
            focus = Focus::Command;
        }
        else if (focus == Focus::Editor && editor.get_mode() == EditorMode::Insert)
        {
            editor.handle_input(':');
        }
        break;

    case '\t':
        if (focus == Focus::Editor)
        {
            focus = Focus::Sidebar;
            editor.enter_normal_mode();
        }
        else
        {
            focus = Focus::Editor;
        }
        break;

    case 27:
        if (focus == Focus::Editor)
            editor.enter_normal_mode();
        break;

    case '\n':
    case KEY_ENTER:
        if (focus != Focus::Sidebar)
            break;

        {
            fs::path path = sidebar.get_selected_path();
            if (path.empty())
                break;

            if (fs::is_directory(path))
            {
                sidebar.toggle_expand(path);
                break;
            }

            if (fs::is_regular_file(path))
            {
                editor.buffer.set_buffer_path(path);
                editor.set_cursor_position(0, 0);
                editor.enter_normal_mode();
                statusbar.set_filename(path.filename().string());
                header.refresh_git(path.parent_path());
                focus = Focus::Editor;
            }
        }
        break;

    case 19:
        if (focus == Focus::Editor)
            editor.buffer.save();
        break;

    default:
        if (focus == Focus::Editor)
            editor.handle_input(ch);
        else if (focus == Focus::Sidebar)
            sidebar.handle_input(ch);
        break;
    }
}

Dimentions UI::get_editor_dim() const
{
    Dimentions d;
    getmaxyx(stdscr, d.height, d.width);
    return d;
}
