#include <ncurses.h>
#include "utils/logger.hpp"
#include "components/statusbar.hpp"
#include "components/header.hpp"
#include "components/editor.hpp"
#include "components/sidebar.hpp"
#include <filesystem>

namespace fs = std::filesystem;

struct Dimentions
{
    int height;
    int width;
};

class UI
{
private:
    Logger l;
    Header header;
    Sidebar sidebar;
    Editor editor;
    Statusbar statusbar;

    int height;
    int width;

    int sidebarWidth = 25;
    bool running = true;
    void init();
    void resize();
    void render();
    void handle_inputs();

public:
    UI(const fs::path filePath);
    ~UI();

    void run();

    Dimentions getEditorDim() const;
};