#include "ui/ui.hpp"

UI::UI()
{
    l.debug("Constructor called");

    header = nullptr;
    sidebar = nullptr;
    editor = nullptr;
    statusbar = nullptr;

    init();
}

UI::~UI()
{
    if (header)
        delwin(header);

    if (sidebar)
        delwin(sidebar);

    if (editor)
        delwin(editor);

    if (statusbar)
        delwin(statusbar);

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
    }

    resize();
}

void UI::resize()
{
    height = getEditorDim().height;
    width = getEditorDim().width;

    // Temporarily print dimensions directly to stdscr
    mvprintw(
        0,
        0,
        "H=%d W=%d Sidebar=%d",
        height,
        width,
        sidebarWidth);

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

    if (header)
    {
        delwin(header);
        header = nullptr;
    }

    if (sidebar)
    {
        delwin(sidebar);
        sidebar = nullptr;
    }

    if (editor)
    {
        delwin(editor);
        editor = nullptr;
    }

    if (statusbar)
    {
        delwin(statusbar);
        statusbar = nullptr;
    }

    int contentHeight = height - 2;
    int editorWidth = width - sidebarWidth;

    header = newwin(
        1,
        width,
        0,
        0);

    sidebar = newwin(
        contentHeight,
        sidebarWidth,
        1,
        0);

    editor = newwin(
        contentHeight,
        editorWidth,
        1,
        sidebarWidth);

    statusbar = newwin(
        1,
        width,
        height - 1,
        0);

    mvprintw(
        1,
        0,
        "header=%p sidebar=%p editor=%p status=%p",
        (void *)header,
        (void *)sidebar,
        (void *)editor,
        (void *)statusbar);

    refresh();
}

void UI::render()
{
    /*
     * Clear every window.
     */
    werase(header);
    werase(sidebar);
    werase(editor);
    werase(statusbar);

    /*
     * Draw everything.
     */
    drawHeader();
    drawSidebar();
    drawEditor();
    drawStatusBar();

    /*
     * Copy windows to ncurses virtual screen.
     */
    wnoutrefresh(header);
    wnoutrefresh(sidebar);
    wnoutrefresh(editor);
    wnoutrefresh(statusbar);

    /*
     * Update the real terminal once.
     */
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

void UI::drawHeader()
{
    // if (!header)
    //     return;

    // if (has_colors())
    //     wbkgd(header, COLOR_PAIR(1));

    // mvwprintw(
    //     header,
    //     0,
    //     1,
    //     "NONI Editor");
}

void UI::drawSidebar()
{
    // if (!sidebar)
    //     return;

    // if (has_colors())
    //     wbkgd(sidebar, COLOR_PAIR(1));

    // mvwprintw(
    //     sidebar,
    //     0,
    //     1,
    //     "Files");

    // mvwprintw(
    //     sidebar,
    //     1,
    //     1,
    //     "main.cpp");
}

void UI::drawEditor()
{
    // if (!editor)
    //     return;

    // if (has_colors())
    //     wbkgd(editor, COLOR_PAIR(1));

    // mvwprintw(
    //     editor,
    //     0,
    //     1,
    //     "Welcome to NONI");

    // mvwprintw(
    //     editor,
    //     1,
    //     1,
    //     "Start editing...");
}

void UI::drawStatusBar()
{
    if (!statusbar)
        return;

    if (has_colors())
    {
        wbkgd(
            statusbar,
            COLOR_PAIR(2));
    }

    /*
     * Left side
     */
    const std::string mode = " NORMAL ";

    mvwprintw(
        statusbar,
        0,
        1,
        "%s",
        mode.c_str());

    /*
     * Cursor position
     */
    const std::string position = "Ln 1, Col 1";

    mvwprintw(
        statusbar,
        0,
        15,
        "%s",
        position.c_str());

    /*
     * Encoding
     */
    const std::string encoding = "UTF-8";

    mvwprintw(
        statusbar,
        0,
        30,
        "%s",
        encoding.c_str());

    /*
     * Filename on the right.
     */
    const std::string filename = "main.cpp";

    int filenameX =
        width -
        static_cast<int>(filename.length()) -
        2;

    /*
     * Don't allow filename to go outside
     * the status bar.
     */
    if (filenameX < 0)
        filenameX = 0;

    mvwprintw(
        statusbar,
        0,
        filenameX,
        "%s",
        filename.c_str());
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