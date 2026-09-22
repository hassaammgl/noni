#include "buffer_detail.hpp"

using namespace buffer_detail;

void Buffer::begin_edit(int cursor_line, int cursor_col)
{
    if (edit_active)
        end_edit(cursor_line, cursor_col);

    edit_active = true;
    open_edit = {};
    open_edit.cursor_before_line = cursor_line;
    open_edit.cursor_before_col = cursor_col;
    open_edit.id_before = content_id;
}

void Buffer::end_edit(int cursor_line, int cursor_col)
{
    if (!edit_active)
        return;

    edit_active = false;
    if (open_edit.changes.empty())
    {
        open_edit = {};
        return;
    }

    open_edit.cursor_after_line = cursor_line;
    open_edit.cursor_after_col = cursor_col;
    open_edit.id_after = next_content_id++;
    content_id = open_edit.id_after;

    undo_stack.push_back(std::move(open_edit));
    open_edit = {};
    redo_stack.clear();
    trim_undo_stack();
}

bool Buffer::is_edit_active() const
{
    return edit_active;
}

void Buffer::trim_undo_stack()
{
    while (undo_stack.size() > kMaxUndo)
        undo_stack.erase(undo_stack.begin());
}

void Buffer::raw_insert_char(int line, int column, char ch)
{
    ensure_line_exists(line);
    column = clamp_column(line, column);
    content[static_cast<std::size_t>(line)].insert(
        static_cast<std::size_t>(column), 1, ch);
}

std::pair<int, int> Buffer::raw_insert_text(int line, int column, std::string_view text)
{
    ensure_line_exists(line);
    column = clamp_column(line, column);

    if (text.empty())
        return {line, column};

    std::vector<std::string> parts;
    std::string cur;
    parts.reserve(8);

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '\r')
        {
            if (i + 1 < text.size() && text[i + 1] == '\n')
                continue;
            parts.push_back(std::move(cur));
            cur.clear();
            continue;
        }
        if (c == '\n')
        {
            parts.push_back(std::move(cur));
            cur.clear();
            continue;
        }
        if (c == '\t' || c >= 32)
            cur.push_back(static_cast<char>(c));
    }
    parts.push_back(std::move(cur));

    if (parts.size() == 1)
    {
        content[static_cast<std::size_t>(line)].insert(
            static_cast<std::size_t>(column),
            parts[0]);
        return {line, column + static_cast<int>(parts[0].size())};
    }

    std::string &first = content[static_cast<std::size_t>(line)];
    const std::string tail = first.substr(static_cast<std::size_t>(column));
    first.erase(static_cast<std::size_t>(column));
    first += parts[0];

    for (std::size_t i = 1; i + 1 < parts.size(); ++i)
    {
        content.insert(
            content.begin() + static_cast<std::ptrdiff_t>(line) + static_cast<std::ptrdiff_t>(i),
            parts[i]);
    }

    const std::size_t last_idx = static_cast<std::size_t>(line) + parts.size() - 1;
    content.insert(
        content.begin() + static_cast<std::ptrdiff_t>(last_idx),
        parts.back() + tail);

    return {
        static_cast<int>(last_idx),
        static_cast<int>(parts.back().size())};
}

void Buffer::raw_insert_newline(int line, int column)
{
    ensure_line_exists(line);
    column = clamp_column(line, column);
    std::string &text = content[static_cast<std::size_t>(line)];
    const std::string tail = text.substr(static_cast<std::size_t>(column));
    text.erase(static_cast<std::size_t>(column));
    content.insert(content.begin() + line + 1, tail);
}

void Buffer::raw_insert_empty_line(int line)
{
    if (line < 0)
        line = 0;
    if (line > static_cast<int>(content.size()))
        line = static_cast<int>(content.size());
    content.insert(content.begin() + line, "");
}

void Buffer::raw_delete_char_before(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    std::string &text = content[static_cast<std::size_t>(line)];
    if (column > 0)
    {
        const std::size_t end = static_cast<std::size_t>(std::min(column, static_cast<int>(text.size())));
        const std::size_t start = TextMetrics::prev_cp(text, end);
        text.erase(start, end - start);
        return;
    }
    if (line == 0)
        return;

    content[static_cast<std::size_t>(line - 1)] += text;
    content.erase(content.begin() + line);
}

void Buffer::raw_delete_char_at(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    std::string &text = content[static_cast<std::size_t>(line)];
    if (column < 0 || column >= static_cast<int>(text.size()))
        return;
    const std::size_t start = TextMetrics::snap_byte(text, static_cast<std::size_t>(column));
    const std::size_t end = TextMetrics::next_cp(text, start);
    text.erase(start, end - start);
}

void Buffer::raw_delete_text(int line, int column, std::string_view text)
{
    int l = line;
    int c = column;
    for (char ch : text)
    {
        if (ch == '\n')
        {
            if (l < 0 || l + 1 >= static_cast<int>(content.size()))
                return;
            if (c != static_cast<int>(content[static_cast<std::size_t>(l)].size()))
                return;
            content[static_cast<std::size_t>(l)] += content[static_cast<std::size_t>(l + 1)];
            content.erase(content.begin() + l + 1);
            continue;
        }

        if (l < 0 || l >= static_cast<int>(content.size()))
            return;
        auto &row = content[static_cast<std::size_t>(l)];
        if (c < 0 || c >= static_cast<int>(row.size()) || row[static_cast<std::size_t>(c)] != ch)
            return;
        row.erase(static_cast<std::size_t>(c), 1);
    }
}

