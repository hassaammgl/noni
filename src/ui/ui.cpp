#include "ui/ui.hpp"

UI::UI(const fs::path filepath = "")
{
    l.debug("Constructor called");

    if (filepath == "")
    {
        l.info("no file path provided");
    }
    else
    {
        std::string filename = filepath.filename().string();
        statusbar.setFilename(filename);
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

    cbreak();
    noecho();

    keypad(stdscr, TRUE);

    curs_set(1);

    if (has_colors())
    {
        start_color();
        use_default_colors();

        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_BLACK, COLOR_WHITE);
        init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
    }

    resize();
}

void UI::resize()
{
    height = getEditorDim().height;
    width = getEditorDim().width;

    refresh();

    if (height < 3 || width <= sidebarWidth)
    {
        mvprintw(
            1,
            0,
            "INVALID DIMENSIONS");

        refresh();

        return;
    }

    int contentHeight = height - 2;
    int editorWidth = width - sidebarWidth;

    sidebar.resize(contentHeight, sidebarWidth, 1, 0);
    editor.resize(contentHeight, editorWidth, 1, sidebarWidth);
    statusbar.resize(1, width, height - 1, 0);
    header.resize(1, width, 0, 0);

    refresh();
}

void UI::render()
{

    header.draw();
    sidebar.draw();
    editor.draw();
    statusbar.draw();

    wnoutrefresh(header.getWindow());
    wnoutrefresh(sidebar.getWindow());
    wnoutrefresh(editor.getWindow());
    wnoutrefresh(statusbar.getWindow());

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

    case KEY_RESIZE:
    {
        resize();
        break;
    }

    default:
    {
        break;
    }
    }
}

Dimentions UI::getEditorDim() const
{
    Dimentions d;

    getmaxyx(
        stdscr,
        d.height,
        d.width);

    return d;
}