#include <ui/ui.hpp>
#include <ui/theme.hpp>
#include <locale>
#include <format>

UI::UI(const fs::path file_path = "")
{
    Logger::debug("UI constructor called");

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
    Logger::debug("Initializing ncurses");
    initscr();

    setlocale(LC_ALL, "");
    cbreak();
    noecho();

    keypad(stdscr, TRUE);

    curs_set(1);

    if (has_colors())
    {
        Theme::init();
    }
    else
    {
        Logger::warning("Terminal has no color support");
    }

    resize();
}

void UI::resize()
{
    height = get_editor_dim().height;
    width = get_editor_dim().width;

    refresh();

    if (height < 3 || width <= sidebar_width)
    {
        Logger::warning(std::format(
            "Invalid dimensions: {}x{} (sidebar width {})",
            width,
            height,
            sidebar_width));
        mvprintw(
            1,
            0,
            "INVALID DIMENSIONS");

        refresh();

        return;
    }

    int content_height = height - 2;
    int editor_width = width - sidebar_width;

    Logger::debug(std::format(
        "Resize layout: {}x{}, editor {}x{}",
        width,
        height,
        editor_width,
        content_height));

    sidebar.resize(content_height, sidebar_width, 1, 0);
    editor.resize(content_height, editor_width, 1, sidebar_width);
    statusbar.resize(1, width, height - 1, 0);
    header.resize(1, width, 0, 0);

    refresh();
}

void UI::render()
{
    Cursor c = editor.get_cursor();
    statusbar.set_cursor_position(c.line + 1, c.column + 1);
    header.draw();
    sidebar.draw();
    statusbar.draw();
    editor.draw();

    wnoutrefresh(header.get_window());
    wnoutrefresh(sidebar.get_window());
    wnoutrefresh(statusbar.get_window());
    wnoutrefresh(editor.get_window());
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

void UI::handle_inputs()
{
    int ch = getch();

    switch (ch)
    {
    case 'q':
    {
        Logger::info("Quit requested");
        running = false;
        break;
    }
    case '\t':
    {
        focus = (focus == Focus::Editor) ? Focus::Sidebar : Focus::Editor;
        Logger::info(std::format(
            "Focus switched to {}",
            focus == Focus::Editor ? "Editor" : "Sidebar"));
        break;
    }
    case '\n':
    case KEY_ENTER:
    {
        if (focus != Focus::Sidebar)
            break;

        fs::path path = sidebar.get_selected_path();
        if (path.empty())
        {
            Logger::debug("Enter pressed in sidebar with no selection");
            break;
        }

        if (fs::is_directory(path))
        {
            sidebar.toggle_expand(path);
            break;
        }

        if (fs::is_regular_file(path))
        {
            Logger::info(std::format("Opening file from sidebar: {}", path.string()));
            editor.buffer.set_buffer_path(path);
            editor.set_cursor_position(0, 0);
            statusbar.set_filename(path.filename().string());
            focus = Focus::Editor;
        }
        break;
    }
    case KEY_RESIZE:
    {
        Logger::debug("Terminal resize event");
        resize();
        break;
    }
    case 19: // Ctrl+S
    {
        if (focus == Focus::Editor)
            editor.buffer.save();
        break;
    }
    default:
    {
        if (focus == Focus::Editor)
        {
            editor.handle_input(ch);
        }
        else if (focus == Focus::Sidebar)
        {
            sidebar.handle_input(ch);
        }
    }
    break;
    }
}

Dimentions UI::get_editor_dim() const
{
    Dimentions d;

    getmaxyx(stdscr, d.height, d.width);

    return d;
}
