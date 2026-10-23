#include "editor_impl.hpp"
#include <utils/messages.hpp>

#include <algorithm>

namespace
{
    std::string join_lines(const Buffer &buf, int lo, int hi)
    {
        std::string block;
        const auto &lines = buf.lines();
        for (int i = lo; i <= hi; ++i)
        {
            block += lines[static_cast<std::size_t>(i)];
            block.push_back('\n');
        }
        return block;
    }
}

void Editor::cmd_copy_lines_down()
{
    if (!tab || !tab->active_window().has_buffer())
        return;

    int lo = tab->cursor().line;
    int hi = lo;
    if (in_visual())
    {
        const TextRange r = tab->active_window().selected_range();
        lo = std::min(r.start.line, r.end.line);
        hi = std::max(r.start.line, r.end.line);
        leave_visual();
    }

    auto &buf = tab->buffer();
    const int n = static_cast<int>(buf.lines().size());
    if (n == 0)
        return;
    lo = std::clamp(lo, 0, n - 1);
    hi = std::clamp(hi, 0, n - 1);
    const std::string block = join_lines(buf, lo, hi);

    begin_buffer_edit();
    if (hi + 1 >= static_cast<int>(buf.lines().size()))
        buf.insert_empty_line(static_cast<int>(buf.lines().size()));
    // Insert at start of line after block; replace blank if we just created EOF blank.
    if (hi + 1 < static_cast<int>(buf.lines().size()) && buf.lines()[static_cast<std::size_t>(hi + 1)].empty())
        buf.delete_range(hi + 1, 0, hi + 1, 0);
    (void)buf.insert_text(hi + 1, 0, block);
    tab->cursor().line = hi + 1;
    tab->cursor().column = 0;
    end_buffer_edit();
    clamp_cursor();
    update_scroll();
}

void Editor::cmd_move_lines_down()
{
    if (!tab || !tab->active_window().has_buffer())
        return;

    int lo = tab->cursor().line;
    int hi = lo;
    if (in_visual())
    {
        const TextRange r = tab->active_window().selected_range();
        lo = std::min(r.start.line, r.end.line);
        hi = std::max(r.start.line, r.end.line);
        leave_visual();
    }

    auto &buf = tab->buffer();
    const int n = static_cast<int>(buf.lines().size());
    if (n == 0 || hi + 1 >= n)
        return;
    lo = std::clamp(lo, 0, n - 1);
    hi = std::clamp(hi, 0, n - 1);

    const std::string block = join_lines(buf, lo, hi);
    const std::string below = buf.lines()[static_cast<std::size_t>(hi + 1)] + "\n";

    begin_buffer_edit();
    buf.delete_range(lo, 0, hi + 2, 0); // exclusive end line hi+2
    (void)buf.insert_text(lo, 0, below + block);
    tab->cursor().line = lo + 1;
    tab->cursor().column = 0;
    end_buffer_edit();
    clamp_cursor();
    update_scroll();
}

void Editor::cmd_move_lines_up()
{
    if (!tab || !tab->active_window().has_buffer())
        return;

    int lo = tab->cursor().line;
    int hi = lo;
    if (in_visual())
    {
        const TextRange r = tab->active_window().selected_range();
        lo = std::min(r.start.line, r.end.line);
        hi = std::max(r.start.line, r.end.line);
        leave_visual();
    }

    auto &buf = tab->buffer();
    if (buf.lines().empty() || lo <= 0)
        return;
    const int n = static_cast<int>(buf.lines().size());
    lo = std::clamp(lo, 0, n - 1);
    hi = std::clamp(hi, 0, n - 1);

    const std::string above = buf.lines()[static_cast<std::size_t>(lo - 1)] + "\n";
    const std::string block = join_lines(buf, lo, hi);

    begin_buffer_edit();
    buf.delete_range(lo - 1, 0, hi + 1, 0);
    (void)buf.insert_text(lo - 1, 0, block + above);
    tab->cursor().line = lo - 1;
    tab->cursor().column = 0;
    end_buffer_edit();
    clamp_cursor();
    update_scroll();
}

bool Editor::jump_diagnostic(bool next)
{
    if (!tab || !tab->active_window().has_buffer())
        return false;

    const auto &items = tab->buffer().diagnostics().items;
    if (items.empty())
    {
        Messages::info("No diagnostics");
        return false;
    }

    std::vector<const Diagnostic *> sorted;
    sorted.reserve(items.size());
    for (const auto &d : items)
        sorted.push_back(&d);
    std::sort(sorted.begin(), sorted.end(), [](const Diagnostic *a, const Diagnostic *b) {
        if (a->start.line != b->start.line)
            return a->start.line < b->start.line;
        return a->start.column < b->start.column;
    });

    const Cursor cur = tab->cursor();
    int idx = -1;
    if (next)
    {
        for (int i = 0; i < static_cast<int>(sorted.size()); ++i)
        {
            const auto &s = sorted[static_cast<std::size_t>(i)]->start;
            if (s.line > cur.line || (s.line == cur.line && s.column > cur.column))
            {
                idx = i;
                break;
            }
        }
        if (idx < 0)
            idx = 0;
    }
    else
    {
        for (int i = static_cast<int>(sorted.size()) - 1; i >= 0; --i)
        {
            const auto &s = sorted[static_cast<std::size_t>(i)]->start;
            if (s.line < cur.line || (s.line == cur.line && s.column < cur.column))
            {
                idx = i;
                break;
            }
        }
        if (idx < 0)
            idx = static_cast<int>(sorted.size()) - 1;
    }

    const Diagnostic &d = *sorted[static_cast<std::size_t>(idx)];
    record_jump_from_here();
    tab->cursor() = d.start;
    clamp_cursor();
    update_scroll();
    Messages::info(d.message);
    return true;
}
