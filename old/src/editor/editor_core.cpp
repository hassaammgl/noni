#include <editor/editor_core.hpp>
#include <utils/text_metrics.hpp>

#include "editor_core_detail.hpp"

using namespace editor_core_detail;

void EditorCore::record_jump_from_active()
{
    Window *w = active_window();
    Buffer *b = active_buffer();
    if (!w || !b)
        return;
    jumps_.push(buffer_id(b), w->cursor());
}

void EditorCore::move_active_to_match(const BufferSearchMatch &m, bool record_jump)
{
    Window *w = active_window();
    if (!w)
        return;
    if (record_jump)
        record_jump_from_active();
    w->cursor() = m.start;
    set_preferred_from_cursor(*w);
    w->clear_selection();
}

std::string EditorCore::search_start(std::string pattern, SearchDirection dir)
{
    Buffer *b = active_buffer();
    Window *w = active_window();
    if (!b || !w)
        return "No buffer";

    BufferSearchQuery q;
    q.pattern = std::move(pattern);
    q.direction = dir;
    q.options = {};

    if (!search_.run(b->lines(), buffer_id(b), b->get_revision(), std::move(q), w->cursor()))
        return search_.last_error().empty() ? "Pattern not found" : search_.last_error();

    if (const BufferSearchMatch *m = search_.current_match())
        move_active_to_match(*m, true);

    return {};
}

std::string EditorCore::search_next(bool reverse)
{
    Buffer *b = active_buffer();
    Window *w = active_window();
    if (!b || !w)
        return "No buffer";

    if (!search_.active() || search_.query().pattern.empty())
        return "No previous search";

    if (!search_.refresh_if_needed(b->lines(), buffer_id(b), b->get_revision()))
    {
        if (!search_.active() || search_.matches().empty())
            return "Pattern not found";
    }

    const bool go_forward =
        (search_.query().direction == SearchDirection::Forward) ? !reverse : reverse;

    Cursor from = w->cursor();
    if (go_forward)
    {
        from.column += 1;
    }
    else if (from.column > 0)
    {
        from.column -= 1;
    }
    else if (from.line > 0)
    {
        from.line -= 1;
        const auto &lines = b->lines();
        if (from.line < static_cast<int>(lines.size()))
            from.column = static_cast<int>(lines[static_cast<std::size_t>(from.line)].size());
    }

    const SearchDirection dir =
        go_forward ? SearchDirection::Forward : SearchDirection::Backward;
    const int idx = BufferSearch::find_next_index(search_.matches(), from, dir, true);
    if (idx < 0)
        return "Pattern not found";

    search_.set_current_index(idx);
    if (const BufferSearchMatch *m = search_.current_match())
        move_active_to_match(*m, true);

    return {};
}

std::string EditorCore::replace_current(std::string_view replacement)
{
    Buffer *b = active_buffer();
    Window *w = active_window();
    if (!b || !w)
        return "No buffer";

    if (!search_.refresh_if_needed(b->lines(), buffer_id(b), b->get_revision()))
    {
        if (!search_.current_match())
            return "No match";
    }

    const BufferSearchMatch *m = search_.current_match();
    if (!m)
        return "No match";

    const BufferSearchMatch match = *m;
    b->begin_edit(w->cursor().line, w->cursor().column);
    b->delete_range(match.start.line, match.start.column, match.end.line, match.end.column);
    const auto [el, ec] = b->insert_text(match.start.line, match.start.column, replacement);
    w->cursor() = {.line = el, .column = ec};
    set_preferred_from_cursor(*w);
    b->end_edit(w->cursor().line, w->cursor().column);

    // Recompute matches after edit; land on next forward match.
    BufferSearchQuery q = search_.query();
    search_.run(b->lines(), buffer_id(b), b->get_revision(), q, w->cursor());
    return {};
}

std::string EditorCore::replace_all(std::string_view replacement)
{
    Buffer *b = active_buffer();
    Window *w = active_window();
    if (!b || !w)
        return "No buffer";

    BufferSearchQuery q = search_.query();
    if (q.pattern.empty())
        return "No previous search";

    auto matches = BufferSearch::find_all(b->lines(), q);
    if (matches.empty())
        return "Pattern not found";

    std::sort(matches.begin(), matches.end(), [](const BufferSearchMatch &a, const BufferSearchMatch &bm) {
        if (a.start.line != bm.start.line)
            return a.start.line > bm.start.line;
        return a.start.column > bm.start.column;
    });

    const int count = static_cast<int>(matches.size());
    b->begin_edit(w->cursor().line, w->cursor().column);
    for (const auto &m : matches)
    {
        b->delete_range(m.start.line, m.start.column, m.end.line, m.end.column);
        (void)b->insert_text(m.start.line, m.start.column, replacement);
    }
    b->end_edit(w->cursor().line, w->cursor().column);

    search_.run(b->lines(), buffer_id(b), b->get_revision(), q, w->cursor());
    (void)count;
    return {};
}

std::string EditorCore::replace_selection(std::string_view replacement)
{
    Buffer *b = active_buffer();
    Window *w = active_window();
    if (!b || !w)
        return "No buffer";
    if (!w->has_selection())
        return "No selection";

    TextRange range = w->selected_range();
    if (range.empty())
        return "No selection";

    b->begin_edit(w->cursor().line, w->cursor().column);
    b->delete_range(range.start.line, range.start.column, range.end.line, range.end.column);
    const auto [el, ec] = b->insert_text(range.start.line, range.start.column, replacement);
    w->cursor() = {.line = el, .column = ec};
    set_preferred_from_cursor(*w);
    w->clear_selection();
    if (buffers_.has_tabs() &&
        (buffers_.active().mode == EditorMode::Visual ||
         buffers_.active().mode == EditorMode::VisualLine))
    {
        buffers_.active().mode = EditorMode::Normal;
    }
    b->end_edit(w->cursor().line, w->cursor().column);
    return {};
}
