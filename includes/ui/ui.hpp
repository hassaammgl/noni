#pragma once

#include <ncurses.h>
#include <utils/logger.hpp>
#include <components/statusbar.hpp>
#include <components/header.hpp>
#include <components/editor.hpp>
#include <components/sidebar.hpp>
#include <filesystem>

namespace fs = std::filesystem;

struct Dimentions
{
    int height;
    int width;
};

enum class Focus
{
    Editor,
    Sidebar,
    Command,
};

class UI
{
private:
    Header header;
    Sidebar sidebar;
    Statusbar statusbar;
    Editor editor;

    int height;
    int width;

    int sidebar_width = 25;
    bool running = true;
    void init();
    void resize();
    void render();
    void handle_inputs();

public:
    Focus focus = Focus::Editor;
    UI(const fs::path file_path);
    ~UI();

    void run();

    Dimentions get_editor_dim() const;
};
