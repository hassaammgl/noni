#pragma once

#include <editor/buffer.hpp>
#include <editor/window.hpp>
#include <editor/window_layout.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

enum class EditorMode
{
    Normal,
    Insert,
    Visual,
    VisualLine,
};

// A tab owns a WindowLayout (one or more Windows). Buffer lifetime is BufferManager's.
struct EditorTab
{
    WindowLayout layout;
    EditorMode mode = EditorMode::Normal;

    Window *active_window_ptr() { return layout.active(); }
    const Window *active_window_ptr() const { return layout.active(); }

    Window &active_window() { return *layout.active(); }
    const Window &active_window() const { return *layout.active(); }

    Buffer &buffer() { return active_window().buffer(); }
    const Buffer &buffer() const { return active_window().buffer(); }

    Cursor &cursor() { return active_window().cursor(); }
    const Cursor &cursor() const { return active_window().cursor(); }

    int &scroll_y() { return active_window().scroll_y(); }
    int scroll_y() const { return active_window().scroll_y(); }

    int &scroll_x() { return active_window().scroll_x(); }
    int scroll_x() const { return active_window().scroll_x(); }

    std::string display_name() const;
    bool is_untitled() const;
};
