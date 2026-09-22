#include "buffer_detail.hpp"

using namespace buffer_detail;

void Buffer::delete_char_before(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    TextChange change;
    if (column > 0)
    {
        const auto &row = content[static_cast<std::size_t>(line)];
        const std::size_t end = static_cast<std::size_t>(std::min(column, static_cast<int>(row.size())));
        const std::size_t start = TextMetrics::prev_cp(row, end);
        change.start_line = line;
        change.start_col = static_cast<int>(start);
        change.deleted = row.substr(start, end - start);
    }
    else
    {
        if (line == 0)
            return;
        change.start_line = line - 1;
        change.start_col = static_cast<int>(content[static_cast<std::size_t>(line - 1)].size());
        change.deleted = "\n";
    }
    record_change(std::move(change));
}

void Buffer::delete_char_at(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    const auto &row = content[static_cast<std::size_t>(line)];
    if (column < 0 || column >= static_cast<int>(row.size()))
        return;

    const std::size_t start = TextMetrics::snap_byte(row, static_cast<std::size_t>(column));
    const std::size_t end = TextMetrics::next_cp(row, start);
    TextChange change;
    change.start_line = line;
    change.start_col = static_cast<int>(start);
    change.deleted = row.substr(start, end - start);
    record_change(std::move(change));
}

std::string Buffer::get_range_text(int start_line, int start_col, int end_line, int end_col) const
{
    if (content.empty())
        return {};

    if (start_line < 0)
        start_line = 0;
    if (start_col < 0)
        start_col = 0;

    if (end_line > static_cast<int>(content.size()))
        end_line = static_cast<int>(content.size());
    if (end_line == static_cast<int>(content.size()))
        end_col = 0;

    if (start_line >= static_cast<int>(content.size()))
        return {};

    if (start_line > end_line)
        return {};
    if (start_line == end_line && start_col >= end_col)
        return {};

    start_col = clamp_column(start_line, start_col);

    std::string out;

    // Through EOF (linewise last lines): no trailing newline after final line.
    if (end_line >= static_cast<int>(content.size()))
    {
        out.append(content[static_cast<std::size_t>(start_line)].substr(
            static_cast<std::size_t>(start_col)));
        for (int line = start_line + 1; line < static_cast<int>(content.size()); ++line)
        {
            out.push_back('\n');
            out.append(content[static_cast<std::size_t>(line)]);
        }
        return out;
    }

    end_col = clamp_column(end_line, end_col);

    if (start_line == end_line)
    {
        const auto &row = content[static_cast<std::size_t>(start_line)];
        return row.substr(
            static_cast<std::size_t>(start_col),
            static_cast<std::size_t>(end_col - start_col));
    }

    out.append(content[static_cast<std::size_t>(start_line)].substr(
        static_cast<std::size_t>(start_col)));
    out.push_back('\n');

    for (int line = start_line + 1; line < end_line; ++line)
    {
        out.append(content[static_cast<std::size_t>(line)]);
        out.push_back('\n');
    }

    out.append(content[static_cast<std::size_t>(end_line)].substr(
        0, static_cast<std::size_t>(end_col)));
    return out;
}

void Buffer::delete_range(int start_line, int start_col, int end_line, int end_col)
{
    const std::string deleted = get_range_text(start_line, start_col, end_line, end_col);
    if (deleted.empty())
        return;

    TextChange change;
    change.start_line = start_line;
    change.start_col = start_col;
    change.deleted = deleted;
    record_change(std::move(change));
}
