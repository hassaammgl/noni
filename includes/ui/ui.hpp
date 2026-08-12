#include <ncurses.h>
#include "utils/logger.hpp"

struct Dimentions
{
    int height;
    int width;
};

class UI
{
private:
    Logger l;
    WINDOW *header;
    WINDOW *sidebar;
    WINDOW *editor;
    WINDOW *statusbar;

    int height;
    int width;

    int sidebarWidth = 25;
    bool running = true;
    void init();
    void resize();
    void render();
    void drawHeader();
    void drawSidebar();
    void drawEditor();
    void drawStatusBar();

    void handle_inputs();

public:
    UI(/* args */);
    ~UI();

    void run();

    Dimentions getEditorDim() const;
};