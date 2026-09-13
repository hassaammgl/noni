#pragma once

#include <editor/buffer.hpp>
#include <utils/cursor.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

enum class EditorMode
{
    Normal,
    Insert,
};

struct EditorTab
{
    Buffer buffer;
    Cursor cursor = {.line = 0, .column = 0};
    int scroll_y = 0;
    int scroll_x = 0;
    EditorMode mode = EditorMode::Normal;

    std::string display_name() const;
    bool is_untitled() const;
};
