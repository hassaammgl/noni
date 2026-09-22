#include "editor_impl.hpp"

void Editor::move_cursor_up()
{
    if (!tab)
        return;
    apply_motion(Motion::up(win()));
}

void Editor::move_cursor_down()
{
    if (!tab)
        return;
    apply_motion(Motion::down(win()));
}

void Editor::move_cursor_left()
{
    if (!tab)
        return;
    apply_motion(Motion::left(win()));
}

void Editor::move_cursor_right()
{
    if (!tab)
        return;
    apply_motion(Motion::right(win()));
}

int Editor::page_step() const
{
    return std::max(1, height > 1 ? height - 1 : 1);
}

void Editor::page_up()
{
    if (!tab)
        return;
    clamp_cursor();
    const int step = page_step();
    Cursor from = tab->cursor();
    tab->cursor().line = std::max(0, tab->cursor().line - step);
    tab->scroll_y() = std::max(0, tab->scroll_y() - step);
    {
        const auto &lines = tab->buffer().lines();
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
            tab->cursor().column = static_cast<int>(
                TextMetrics::display_to_byte(row, tab->active_window().preferred_column()));
        }
    }
    clamp_cursor();
    if (std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}

void Editor::page_down()
{
    if (!tab)
        return;
    clamp_cursor();
    const auto &lines = tab->buffer().lines();
    if (lines.empty())
        return;
    const int step = page_step();
    const int last = static_cast<int>(lines.size()) - 1;
    Cursor from = tab->cursor();
    tab->cursor().line = std::min(last, tab->cursor().line + step);
    tab->scroll_y() = std::min(
        std::max(0, last - std::max(0, height - 1)),
        tab->scroll_y() + step);
    {
        const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
        tab->cursor().column = static_cast<int>(
            TextMetrics::display_to_byte(row, tab->active_window().preferred_column()));
    }
    clamp_cursor();
    if (std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}

void Editor::half_page_up()
{
    if (!tab)
        return;
    clamp_cursor();
    const int step = std::max(1, page_step() / 2);
    Cursor from = tab->cursor();
    tab->cursor().line = std::max(0, tab->cursor().line - step);
    tab->scroll_y() = std::max(0, tab->scroll_y() - step);
    {
        const auto &lines = tab->buffer().lines();
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
            tab->cursor().column = static_cast<int>(
                TextMetrics::display_to_byte(row, tab->active_window().preferred_column()));
        }
    }
    clamp_cursor();
    if (std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}

void Editor::half_page_down()
{
    if (!tab)
        return;
    clamp_cursor();
    const auto &lines = tab->buffer().lines();
    if (lines.empty())
        return;
    const int step = std::max(1, page_step() / 2);
    const int last = static_cast<int>(lines.size()) - 1;
    Cursor from = tab->cursor();
    tab->cursor().line = std::min(last, tab->cursor().line + step);
    tab->scroll_y() = std::min(
        std::max(0, last - std::max(0, height - 1)),
        tab->scroll_y() + step);
    {
        const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
        tab->cursor().column = static_cast<int>(
            TextMetrics::display_to_byte(row, tab->active_window().preferred_column()));
    }
    clamp_cursor();
    if (std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}

void Editor::update_scroll()
{
    if (!tab || height <= 0)
        return;

    if (tab->cursor().line < tab->scroll_y())
        tab->scroll_y() = tab->cursor().line;

    if (tab->cursor().line >= tab->scroll_y() + height)
        tab->scroll_y() = tab->cursor().line - height + 1;

    if (tab->scroll_y() < 0)
        tab->scroll_y() = 0;

    sync_horizontal_scroll(tab->active_window(), width);
}

void Editor::clamp_cursor()
{
    if (!tab)
        return;

    tab->cursor() = clamp_cursor_to_lines(tab->cursor(), tab->buffer().lines());
}

void Editor::apply_motion(const MotionResult &motion, bool record_jump)
{
    if (!tab)
        return;
    Cursor from = tab->cursor();
    tab->cursor() = motion.dest;
    clamp_cursor();
    if (record_jump && std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}
