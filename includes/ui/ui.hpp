#pragma once

#include <ncurses.h>
#include <components/commandline.hpp>
#include <components/editor.hpp>
#include <components/header.hpp>
#include <components/line_number.hpp>
#include <components/messages_panel.hpp>
#include <components/sidebar.hpp>
#include <components/statusbar.hpp>
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
    Messages,
};

class UI
{
private:
    Header header;
    Sidebar sidebar;
    LineNumber line_number;
    Editor editor;
    MessagesPanel messages_panel;
    Statusbar statusbar;
    CommandLine command_line;

    int height;
    int width;

    int sidebar_width = 25;
    int line_number_width = 5;
    bool running = true;

    void init();
    void resize();
    void render();
    void handle_inputs();
    void update_statusbar_mode();
    void update_cursor_visibility();

public:
    Focus focus = Focus::Editor;

    UI(const fs::path file_path);
    ~UI();

    void run();
    void request_quit();

    CommandLine &get_command_line();
    MessagesPanel &get_messages_panel();
    Editor &get_editor();

    Dimentions get_editor_dim() const;
};
