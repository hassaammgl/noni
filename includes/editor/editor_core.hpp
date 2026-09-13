#pragma once

#include <editor/buffer_manager.hpp>
#include <editor/buffer_search.hpp>
#include <editor/jumps.hpp>
#include <editor/marks.hpp>
#include <editor/registers.hpp>
#include <editor/window_layout.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

// Editor engine state — no ncurses dependency.
class EditorCore
{
private:
    BufferManager buffers_;
    RegisterFile registers_;
    MarkTable marks_;
    JumpList jumps_;
    BufferSearchState search_;
    int editor_area_w_ = 80;
    int editor_area_h_ = 24;

    void record_jump_from_active();
    void move_active_to_match(const BufferSearchMatch &m, bool record_jump);

public:
    BufferManager &buffers() { return buffers_; }
    const BufferManager &buffers() const { return buffers_; }

    RegisterFile &registers() { return registers_; }
    const RegisterFile &registers() const { return registers_; }

    MarkTable &marks() { return marks_; }
    JumpList &jumps() { return jumps_; }

    BufferSearchState &search() { return search_; }
    const BufferSearchState &search() const { return search_; }

    void set_editor_area(int w, int h)
    {
        editor_area_w_ = std::max(1, w);
        editor_area_h_ = std::max(1, h);
    }

    int editor_area_w() const { return editor_area_w_; }
    int editor_area_h() const { return editor_area_h_; }

    bool has_tabs() const { return buffers_.has_tabs(); }

    EditorTab &active_tab() { return buffers_.active(); }
    const EditorTab &active_tab() const { return buffers_.active(); }

    Window *active_window()
    {
        if (!buffers_.has_tabs())
            return nullptr;
        return buffers_.active().active_window_ptr();
    }

    const Window *active_window() const
    {
        if (!buffers_.has_tabs())
            return nullptr;
        return buffers_.active().active_window_ptr();
    }

    Buffer *active_buffer()
    {
        Window *w = active_window();
        return (w && w->has_buffer()) ? &w->buffer() : nullptr;
    }

    const Buffer *active_buffer() const
    {
        const Window *w = active_window();
        return (w && w->has_buffer()) ? &w->buffer() : nullptr;
    }

    EditorMode mode() const
    {
        return buffers_.has_tabs() ? buffers_.active().mode : EditorMode::Normal;
    }

    void set_mode(EditorMode mode)
    {
        if (buffers_.has_tabs())
            buffers_.active().mode = mode;
    }

    std::uintptr_t buffer_id(Buffer *b) const
    {
        return reinterpret_cast<std::uintptr_t>(b);
    }

    void on_buffer_closed(Buffer *b)
    {
        if (!b)
            return;
        const auto id = buffer_id(b);
        marks_.invalidate_buffer(id);
        jumps_.invalidate_buffer(id);
        if (search_.buffer_id() == id)
            search_.clear();
    }

    // Buffer search / replace (P9). Empty string = success.
    std::string search_start(std::string pattern, SearchDirection dir);
    std::string search_next(bool reverse);
    std::string replace_current(std::string_view replacement);
    std::string replace_all(std::string_view replacement);
    std::string replace_selection(std::string_view replacement);

    bool split_vertical()
    {
        if (!has_tabs())
            return false;
        return active_tab().layout.split_vertical();
    }

    bool split_horizontal()
    {
        if (!has_tabs())
            return false;
        return active_tab().layout.split_horizontal();
    }

    bool close_window()
    {
        if (!has_tabs())
            return false;
        return active_tab().layout.close_active();
    }

    bool focus_left()
    {
        if (!has_tabs())
            return false;
        return active_tab().layout.focus_neighbor(-1, 0, editor_area_w_, editor_area_h_);
    }

    bool focus_right()
    {
        if (!has_tabs())
            return false;
        return active_tab().layout.focus_neighbor(1, 0, editor_area_w_, editor_area_h_);
    }

    bool focus_up()
    {
        if (!has_tabs())
            return false;
        return active_tab().layout.focus_neighbor(0, -1, editor_area_w_, editor_area_h_);
    }

    bool focus_down()
    {
        if (!has_tabs())
            return false;
        return active_tab().layout.focus_neighbor(0, 1, editor_area_w_, editor_area_h_);
    }

    void resize_left()
    {
        if (has_tabs())
            active_tab().layout.resize_active(SplitOrientation::Vertical, -0.05f);
    }

    void resize_right()
    {
        if (has_tabs())
            active_tab().layout.resize_active(SplitOrientation::Vertical, 0.05f);
    }

    void resize_up()
    {
        if (has_tabs())
            active_tab().layout.resize_active(SplitOrientation::Horizontal, -0.05f);
    }

    void resize_down()
    {
        if (has_tabs())
            active_tab().layout.resize_active(SplitOrientation::Horizontal, 0.05f);
    }
};
