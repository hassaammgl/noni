#include "buffer_detail.hpp"

using namespace buffer_detail;

void Buffer::record_change(TextChange change)
{
    const bool auto_wrap = !edit_active;
    if (auto_wrap)
        begin_edit(change.start_line, change.start_col);

    // Coalesce contiguous pure inserts into the last change.
    if (!open_edit.changes.empty() && change.deleted.empty() && !change.inserted.empty())
    {
        TextChange &last = open_edit.changes.back();
        if (last.deleted.empty() && !last.inserted.empty())
        {
            int el = last.start_line;
            int ec = last.start_col;
            advance_pos(el, ec, last.inserted, content);
            if (el == change.start_line && ec == change.start_col)
            {
                last.inserted += change.inserted;
                apply_forward(change);
                syntax_engine_.notify_edit(change);
                bump_revision();
                if (change_listener_)
                    change_listener_(*this, change);
                if (auto_wrap)
                    end_edit(change.start_line, change.start_col);
                return;
            }
        }
    }

    // Coalesce repeated deletes at the same start (e.g. S clearing a line).
    if (!open_edit.changes.empty() && change.inserted.empty() && !change.deleted.empty())
    {
        TextChange &last = open_edit.changes.back();
        if (last.inserted.empty() && !last.deleted.empty() &&
            last.start_line == change.start_line && last.start_col == change.start_col)
        {
            last.deleted += change.deleted;
            apply_forward(change);
            syntax_engine_.notify_edit(change);
            bump_revision();
            if (change_listener_)
                change_listener_(*this, change);
            if (auto_wrap)
                end_edit(change.start_line, change.start_col);
            return;
        }
    }

    apply_forward(change);
    syntax_engine_.notify_edit(change);
    if (change_listener_)
        change_listener_(*this, change);
    open_edit.changes.push_back(std::move(change));
    bump_revision();

    if (auto_wrap)
    {
        // Cursor after unknown — caller should use begin/end; for auto use start.
        end_edit(change.start_line, change.start_col);
    }
}

void Buffer::insert_char(int line, int column, char ch)
{
    TextChange change;
    change.start_line = line;
    change.start_col = column;
    change.inserted.assign(1, ch);
    record_change(std::move(change));
}

std::pair<int, int> Buffer::insert_text(int line, int column, std::string_view text)
{
    if (text.empty())
        return {line, column};

    // Normalize to the same filtering as raw_insert_text for the recorded string.
    std::string filtered;
    filtered.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '\r')
        {
            if (i + 1 < text.size() && text[i + 1] == '\n')
                continue;
            filtered.push_back('\n');
            continue;
        }
        if (c == '\n')
        {
            filtered.push_back('\n');
            continue;
        }
        if (c == '\t' || c >= 32)
            filtered.push_back(static_cast<char>(c));
    }

    if (filtered.empty())
        return {line, column};

    TextChange change;
    change.start_line = line;
    change.start_col = column;
    change.inserted = filtered;

    const bool auto_wrap = !edit_active;
    if (auto_wrap)
        begin_edit(line, column);

    apply_forward(change);
    syntax_engine_.notify_edit(change);
    if (change_listener_)
        change_listener_(*this, change);
    open_edit.changes.push_back(std::move(change));
    bump_revision();

    int el = line;
    int ec = column;
    advance_pos(el, ec, filtered, content);

    if (auto_wrap)
        end_edit(el, ec);

    return {el, ec};
}

void Buffer::insert_newline(int line, int column)
{
    TextChange change;
    change.start_line = line;
    change.start_col = column;
    change.inserted = "\n";
    record_change(std::move(change));
}

void Buffer::insert_empty_line(int line)
{
    // Inserting "" at index `line` == inserting '\n' at beginning of that line
    // (or at end of document).
    TextChange change;
    change.start_line = line;
    change.start_col = 0;
    if (line >= static_cast<int>(content.size()))
    {
        // Append: put newline at end of last line.
        if (content.empty())
        {
            content.push_back("");
        }
        change.start_line = static_cast<int>(content.size()) - 1;
        change.start_col = static_cast<int>(content.back().size());
    }
    change.inserted = "\n";
    record_change(std::move(change));
}
