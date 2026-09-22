#include <editor/editor_core.hpp>

void EditorCore::attach_lsp_document(Buffer &buffer)
{
    buffer.set_change_listener([this](Buffer &b, const TextChange &ch) {
        lsp_.notify_change(b, ch);
        events_.emit_buffer_changed(b, ch);
    });
    buffer.set_reload_listener([this](Buffer &b) {
        lsp_.notify_reload(b);
        events_.emit_buffer_changed(b, TextChange{});
    });
    lsp_.notify_open(buffer);
    events_.emit_buffer_opened(buffer);
}

std::optional<ScmFileStatus> EditorCore::scm_status_for_path(const fs::path &path) const
{
    return scm_.status_for(path);
}

void EditorCore::set_editor_area(int w, int h)
{
    editor_area_w_ = std::max(1, w);
    editor_area_h_ = std::max(1, h);
}

Window *EditorCore::active_window()
{
    if (!buffers_.has_tabs())
        return nullptr;
    return buffers_.active().active_window_ptr();
}

const Window *EditorCore::active_window() const
{
    if (!buffers_.has_tabs())
        return nullptr;
    return buffers_.active().active_window_ptr();
}

Buffer *EditorCore::active_buffer()
{
    Window *w = active_window();
    return (w && w->has_buffer()) ? &w->buffer() : nullptr;
}

const Buffer *EditorCore::active_buffer() const
{
    const Window *w = active_window();
    return (w && w->has_buffer()) ? &w->buffer() : nullptr;
}

EditorMode EditorCore::mode() const
{
    return buffers_.has_tabs() ? buffers_.active().mode : EditorMode::Normal;
}

void EditorCore::set_mode(EditorMode mode)
{
    if (buffers_.has_tabs())
        buffers_.active().mode = mode;
}

std::uintptr_t EditorCore::buffer_id(Buffer *b) const
{
    return reinterpret_cast<std::uintptr_t>(b);
}

void EditorCore::on_buffer_closed(Buffer *b)
{
    if (!b)
        return;
    events_.emit_buffer_closed(*b);
    const auto id = buffer_id(b);
    marks_.invalidate_buffer(id);
    jumps_.invalidate_buffer(id);
    if (search_.buffer_id() == id)
        search_.clear();
}

void EditorCore::notify_buffer_saved(Buffer &b)
{
    events_.emit_buffer_saved(b);
}

bool EditorCore::split_vertical()
{
    if (!has_tabs())
        return false;
    return active_tab().layout.split_vertical();
}

bool EditorCore::split_horizontal()
{
    if (!has_tabs())
        return false;
    return active_tab().layout.split_horizontal();
}

bool EditorCore::close_window()
{
    if (!has_tabs())
        return false;
    return active_tab().layout.close_active();
}

bool EditorCore::focus_left()
{
    if (!has_tabs())
        return false;
    return active_tab().layout.focus_neighbor(-1, 0, editor_area_w_, editor_area_h_);
}

bool EditorCore::focus_right()
{
    if (!has_tabs())
        return false;
    return active_tab().layout.focus_neighbor(1, 0, editor_area_w_, editor_area_h_);
}

bool EditorCore::focus_up()
{
    if (!has_tabs())
        return false;
    return active_tab().layout.focus_neighbor(0, -1, editor_area_w_, editor_area_h_);
}

bool EditorCore::focus_down()
{
    if (!has_tabs())
        return false;
    return active_tab().layout.focus_neighbor(0, 1, editor_area_w_, editor_area_h_);
}

void EditorCore::resize_left()
{
    if (has_tabs())
        active_tab().layout.resize_active(SplitOrientation::Vertical, -0.05f);
}

void EditorCore::resize_right()
{
    if (has_tabs())
        active_tab().layout.resize_active(SplitOrientation::Vertical, 0.05f);
}

void EditorCore::resize_up()
{
    if (has_tabs())
        active_tab().layout.resize_active(SplitOrientation::Horizontal, -0.05f);
}

void EditorCore::resize_down()
{
    if (has_tabs())
        active_tab().layout.resize_active(SplitOrientation::Horizontal, 0.05f);
}
