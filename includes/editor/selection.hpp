#pragma once

#include <utils/cursor.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <string>
#include <vector>

// View-layer selection. Never stored on Buffer.
enum class SelectionKind
{
    None,
    Character,
    Line,
};

struct Selection
{
    SelectionKind kind = SelectionKind::None;
    Cursor anchor{.line = 0, .column = 0};

    bool active() const { return kind != SelectionKind::None; }
};

// Ordered inclusive-start / exclusive-end range for edits and yank.
// For Line kind, end may be {line_count, 0} meaning through EOF.
struct TextRange
{
    Cursor start{.line = 0, .column = 0};
    Cursor end{.line = 0, .column = 0};
    SelectionKind kind = SelectionKind::Character;

    bool empty() const
    {
        return start.line == end.line && start.column == end.column;
    }
};

inline bool cursor_before(const Cursor &a, const Cursor &b)
{
    return a.line < b.line || (a.line == b.line && a.column < b.column);
}

inline bool cursor_equal(const Cursor &a, const Cursor &b)
{
    return a.line == b.line && a.column == b.column;
}

inline Cursor clamp_cursor_to_lines(Cursor c, const std::vector<std::string> &lines)
{
    if (lines.empty())
        return {.line = 0, .column = 0};
    if (c.line < 0)
        c.line = 0;
    if (c.line >= static_cast<int>(lines.size()))
        c.line = static_cast<int>(lines.size()) - 1;
    const auto &row = lines[static_cast<std::size_t>(c.line)];
    const int len = static_cast<int>(row.size());
    if (c.column < 0)
        c.column = 0;
    if (c.column > len)
        c.column = len;
    else
        c.column = static_cast<int>(TextMetrics::snap_byte(row, static_cast<std::size_t>(c.column)));
    return c;
}

// Inclusive visual endpoints → exclusive-end character range (byte offsets).
inline TextRange normalize_character_range(
    Cursor a,
    Cursor b,
    const std::vector<std::string> &lines)
{
    TextRange range;
    range.kind = SelectionKind::Character;

    if (lines.empty())
        return range;

    a = clamp_cursor_to_lines(a, lines);
    b = clamp_cursor_to_lines(b, lines);

    Cursor lo = cursor_before(a, b) ? a : b;
    Cursor hi = cursor_before(a, b) ? b : a;
    range.start = lo;

    const auto &hi_row = lines[static_cast<std::size_t>(hi.line)];
    const int hi_len = static_cast<int>(hi_row.size());
    if (hi.column < hi_len)
    {
        range.end = {
            .line = hi.line,
            .column = static_cast<int>(
                TextMetrics::next_cp(hi_row, static_cast<std::size_t>(hi.column)))};
    }
    else if (hi.line + 1 < static_cast<int>(lines.size()))
    {
        // At EOL: include the newline joining to the next line.
        range.end = {.line = hi.line + 1, .column = 0};
    }
    else
    {
        range.end = {.line = hi.line, .column = hi_len};
    }

    return range;
}

inline TextRange normalize_line_range(Cursor a, Cursor b, int line_count)
{
    TextRange range;
    range.kind = SelectionKind::Line;

    if (line_count <= 0)
        return range;

    int lo = std::clamp(std::min(a.line, b.line), 0, line_count - 1);
    int hi = std::clamp(std::max(a.line, b.line), 0, line_count - 1);

    range.start = {.line = lo, .column = 0};
    range.end = {.line = hi + 1, .column = 0}; // may equal line_count
    return range;
}
