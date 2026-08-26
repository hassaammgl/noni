#include <ui/ui.hpp>
#include <ui/theme.hpp>
#include <locale>

UI::UI(const fs::path file_path = "")
{
    l.debug("Constructor called");

    if (file_path == "")
    {
        l.info("no file path provided");
    }
    else
    {
        std::string filename = file_path.filename().string();
        std::string parent_folder = file_path.parent_path();
        statusbar.set_filename(filename);
        sidebar.set_project_path(parent_folder);
        editor.buffer.set_buffer_path(file_path);
    }

    init();
}

UI::~UI()
{
    endwin();
    l.debug("Destructor called");
}

void UI::init()
{
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

    resize();
}

void UI::resize()
{
    height = get_editor_dim().height;
    width = get_editor_dim().width;

    refresh();

    if (height < 3 || width <= sidebar_width)
    {
        mvprintw(
            1,
            0,
            "INVALID DIMENSIONS");

        refresh();

        return;
    }

    int content_height = height - 2;
    int editor_width = width - sidebar_width;

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
    while (running)
    {
        render();
        handle_inputs();
    }

    l.debug("Runner stopped");
}

void UI::handle_inputs()
{
    int ch = getch();

    switch (ch)
    {
    case 'q':
    {
        running = false;
        break;
    }
    case '\t':
    {
        focus = (focus == Focus::Editor) ? Focus::Sidebar : Focus::Editor;
        break;
    }
    case '\n':
        // case KEY_ENTER:
        {
            if (focus == Focus::Sidebar)
                break;
            fs::path path = sidebar.get_selected_path();
            if (path.empty() || !fs::is_regular_file(path))
            {
                break;
            }
            editor.buffer.set_buffer_path(path);
            editor.set_cursor_position(0, 0);
            statusbar.set_filename(path.filename().string());
            focus = Focus::Editor;
            resize();
            break;
        }
    case KEY_RESIZE:
    {
        resize();
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