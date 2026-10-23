#include "editor_impl.hpp"
#include <editor/comment_tokens.hpp>
#include <syntax/syntax.hpp>
#include <utils/messages.hpp>

#include <algorithm>

namespace
{
    Language lang_of(const Buffer &buf)
    {
        const fs::path p = buf.get_buffer_path();
        if (p.empty())
            return Language::Plain;
        return Syntax::detect_language(p);
    }

    int leading_ws(std::string_view line)
    {
        int i = 0;
        while (i < static_cast<int>(line.size()) &&
               (line[static_cast<std::size_t>(i)] == ' ' || line[static_cast<std::size_t>(i)] == '\t'))
            ++i;
        return i;
    }

    TextRange current_line_range(EditorTab &tab)
    {
        return normalize_line_range(
            tab.cursor(), tab.cursor(), static_cast<int>(tab.buffer().lines().size()));
    }
}

void Editor::cmd_copy_selection_or_line()
{
    if (!tab)
        return;
    TextRange range = in_visual() ? tab->active_window().selected_range() : current_line_range(*tab);
    yank_range(range);
    if (in_visual())
        leave_visual();
    Messages::info("Copied");
}

void Editor::cmd_cut_selection_or_line()
{
    if (!tab)
        return;
    TextRange range = in_visual() ? tab->active_window().selected_range() : current_line_range(*tab);
    execute_operator(PendingOperator::Delete, range, false);
    leave_visual();
    Messages::info("Cut");
}

void Editor::cmd_toggle_comment()
{
    if (!tab || !tab->active_window().has_buffer())
        return;

    const auto prefix = CommentTokens::line_prefix(lang_of(tab->buffer()));
    if (prefix.empty())
    {
        Messages::info("No line comments for this language");
        return;
    }

    int lo = tab->cursor().line;
    int hi = lo;
    if (in_visual())
    {
        const TextRange r = tab->active_window().selected_range();
        lo = std::min(r.start.line, r.end.line);
        hi = std::max(r.start.line, r.end.line);
        leave_visual();
    }

    const auto &lines = tab->buffer().lines();
    if (lines.empty())
        return;
    lo = std::clamp(lo, 0, static_cast<int>(lines.size()) - 1);
    hi = std::clamp(hi, 0, static_cast<int>(lines.size()) - 1);

    const std::string pref(prefix);
    const std::string pref_sp = pref + " ";

    bool all_commented = true;
    for (int i = lo; i <= hi; ++i)
    {
        const auto &line = lines[static_cast<std::size_t>(i)];
        const int ws = leading_ws(line);
        const std::string_view body(line.data() + ws, line.size() - static_cast<std::size_t>(ws));
        if (body.empty())
            continue;
        if (!(body.starts_with(pref_sp) || body.starts_with(pref)))
        {
            all_commented = false;
            break;
        }
    }

    begin_buffer_edit();
    for (int i = lo; i <= hi; ++i)
    {
        const std::string line = tab->buffer().lines()[static_cast<std::size_t>(i)];
        const int ws = leading_ws(line);
        std::string head = line.substr(0, static_cast<std::size_t>(ws));
        std::string body = line.substr(static_cast<std::size_t>(ws));
        if (body.empty())
            continue;
        if (all_commented)
        {
            if (body.starts_with(pref_sp))
                body = body.substr(pref_sp.size());
            else if (body.starts_with(pref))
                body = body.substr(pref.size());
        }
        else
            body = pref_sp + body;

        tab->buffer().delete_range(i, 0, i, static_cast<int>(line.size()));
        (void)tab->buffer().insert_text(i, 0, head + body);
    }
    end_buffer_edit();
    clamp_cursor();
    update_scroll();
}
