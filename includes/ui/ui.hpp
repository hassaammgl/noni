#include <ncurses.h>

class UI
{
private:
    WINDOW *header;
    WINDOW *sidebar;
    WINDOW *editor;
    WINDOW *statusbar;

    int height;
    int width;

    int sidebarWidth = 25;

public:
    UI(/* args */);
    ~UI();

    void init();
    void resize();

    void draw();
    void drawHeader();
    void drawSidebar();
    void drawEditor();
    void drawStatusBar();

    int getEditorWidth() const;
    int getEditorHeight() const;
};