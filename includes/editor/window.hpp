#pragma once

#include <editor/buffer.hpp>
#include <editor/selection.hpp>
#include <utils/cursor.hpp>

#include <algorithm>

// A view onto a Buffer. Does not own the document.
// Future splits: multiple Windows may reference the same Buffer.
class Window
{
private:
    Buffer *buffer_ = nullptr;
    Cursor cursor_{.line = 0, .column = 0};
    int scroll_y_ = 0;
    int scroll_x_ = 0;           // horizontal scroll in DISPLAY columns
    Selection selection_{};
    int preferred_column_ = 0; // preferred DISPLAY column for vertical motion

public:
    Window() = default;
    explicit Window(Buffer *buffer) : buffer_(buffer) {}

    void bind(Buffer *buffer)
    {
        buffer_ = buffer;
        cursor_ = {.line = 0, .column = 0};
        scroll_y_ = 0;
        scroll_x_ = 0;
        preferred_column_ = 0;
        clear_selection();
    }

    bool has_buffer() const { return buffer_ != nullptr; }

    Buffer &buffer() { return *buffer_; }
    const Buffer &buffer() const { return *buffer_; }

    Cursor &cursor() { return cursor_; }
    const Cursor &cursor() const { return cursor_; }

    int &scroll_y() { return scroll_y_; }
    int scroll_y() const { return scroll_y_; }

    int &scroll_x() { return scroll_x_; }
    int scroll_x() const { return scroll_x_; }

    int &preferred_column() { return preferred_column_; }
    int preferred_column() const { return preferred_column_; }

    Selection &selection() { return selection_; }
    const Selection &selection() const { return selection_; }

    bool has_selection() const { return selection_.active(); }

    void clear_selection()
    {
        selection_.kind = SelectionKind::None;
    }

    void enter_visual(SelectionKind kind)
    {
        if (kind == SelectionKind::None)
        {
            clear_selection();
            return;
        }
        selection_.kind = kind;
        selection_.anchor = cursor_;
    }

    TextRange selected_range() const
    {
        if (!buffer_ || !selection_.active())
            return {};

        const auto &lines = buffer_->lines();
        if (selection_.kind == SelectionKind::Line)
            return normalize_line_range(selection_.anchor, cursor_, static_cast<int>(lines.size()));
        return normalize_character_range(selection_.anchor, cursor_, lines);
    }

    // True if (line, col) is inside the visual selection for rendering.
    // Character: inclusive endpoints. Line: whole lines.
    bool is_selected(int line, int col) const
    {
        if (!selection_.active() || !buffer_)
            return false;

        const auto &lines = buffer_->lines();
        if (lines.empty() || line < 0 || line >= static_cast<int>(lines.size()))
            return false;

        if (selection_.kind == SelectionKind::Line)
        {
            const int lo = std::min(selection_.anchor.line, cursor_.line);
            const int hi = std::max(selection_.anchor.line, cursor_.line);
            return line >= lo && line <= hi;
        }

        Cursor a = clamp_cursor_to_lines(selection_.anchor, lines);
        Cursor b = clamp_cursor_to_lines(cursor_, lines);
        Cursor lo = cursor_before(a, b) ? a : b;
        Cursor hi = cursor_before(a, b) ? b : a;

        if (line < lo.line || line > hi.line)
            return false;
        if (lo.line == hi.line)
            return col >= lo.column && col <= hi.column;
        if (line == lo.line)
            return col >= lo.column;
        if (line == hi.line)
            return col <= hi.column;
        return true;
    }

    void reset_view()
    {
        cursor_ = {.line = 0, .column = 0};
        scroll_y_ = 0;
        scroll_x_ = 0;
        preferred_column_ = 0;
        clear_selection();
    }
};
