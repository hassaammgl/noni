#include "buffer_detail.hpp"

#include <utils/text_metrics.hpp>

#include <algorithm>

using namespace buffer_detail;

void Buffer::reset_line_metrics() const
{
    line_disp_width_.clear();
    metrics_hot_line_ = -1;
    metrics_hot_prefix_.clear();
}

void Buffer::ensure_metrics_size() const
{
    const std::size_t n = content.size();
    if (line_disp_width_.size() != n)
        line_disp_width_.assign(n, -1);
}

void Buffer::note_metrics_edit(const TextChange &change)
{
    ensure_metrics_size();
    int nl_del = 0;
    int nl_ins = 0;
    for (char c : change.deleted)
        if (c == '\n')
            ++nl_del;
    for (char c : change.inserted)
        if (c == '\n')
            ++nl_ins;

    if (nl_del == 0 && nl_ins == 0)
    {
        if (change.start_line >= 0 &&
            change.start_line < static_cast<int>(line_disp_width_.size()))
            line_disp_width_[static_cast<std::size_t>(change.start_line)] = -1;
    }
    else
    {
        line_disp_width_.assign(content.size(), -1);
    }

    if (metrics_hot_line_ < 0)
        return;
    if (nl_del != 0 || nl_ins != 0 || metrics_hot_line_ == change.start_line)
        metrics_hot_line_ = -1;
}

void Buffer::ensure_hot_line(int line) const
{
    if (line < 0 || line >= static_cast<int>(content.size()))
    {
        metrics_hot_line_ = -1;
        return;
    }
    ensure_metrics_size();
    if (metrics_hot_line_ == line &&
        metrics_hot_prefix_.size() == content[static_cast<std::size_t>(line)].size() + 1)
        return;

    const std::string_view row = content[static_cast<std::size_t>(line)];
    metrics_hot_prefix_.assign(row.size() + 1, 0);
    int dcol = 0;
    std::size_t i = 0;
    while (i < row.size())
    {
        metrics_hot_prefix_[i] = dcol;
        const auto [cp, n] = TextMetrics::decode(row, i);
        if (n == 0)
            break;
        int w = (cp == U'\t') ? TextMetrics::tab_width_at(dcol) : TextMetrics::codepoint_width(cp);
        if (w < 0)
            w = 1;
        for (std::size_t k = 1; k < n && i + k < metrics_hot_prefix_.size(); ++k)
            metrics_hot_prefix_[i + k] = dcol;
        dcol += w;
        i += n;
    }
    metrics_hot_prefix_[row.size()] = dcol;
    line_disp_width_[static_cast<std::size_t>(line)] = dcol;
    metrics_hot_line_ = line;
}

int Buffer::line_display_width_cached(int line) const
{
    if (line < 0 || line >= static_cast<int>(content.size()))
        return 0;
    ensure_metrics_size();
    int &slot = line_disp_width_[static_cast<std::size_t>(line)];
    if (slot < 0)
    {
        slot = TextMetrics::line_display_width(content[static_cast<std::size_t>(line)]);
    }
    return slot;
}

int Buffer::byte_to_display_cached(int line, std::size_t byte) const
{
    if (line < 0 || line >= static_cast<int>(content.size()))
        return 0;
    ensure_hot_line(line);
    const std::size_t n = content[static_cast<std::size_t>(line)].size();
    if (byte > n)
        byte = n;
    if (byte < metrics_hot_prefix_.size())
        return metrics_hot_prefix_[byte];
    return TextMetrics::byte_to_display(content[static_cast<std::size_t>(line)], byte);
}

std::size_t Buffer::display_to_byte_cached(int line, int display_col) const
{
    if (line < 0 || line >= static_cast<int>(content.size()))
        return 0;
    ensure_hot_line(line);
    const std::string_view row = content[static_cast<std::size_t>(line)];
    if (display_col <= 0)
        return 0;
    // Walk codepoint starts using prefix table.
    std::size_t i = 0;
    while (i < row.size())
    {
        const int d0 = metrics_hot_prefix_[i];
        const auto [cp, n] = TextMetrics::decode(row, i);
        if (n == 0)
            break;
        int w = (cp == U'\t') ? TextMetrics::tab_width_at(d0) : TextMetrics::codepoint_width(cp);
        if (w < 0)
            w = 1;
        if (display_col < d0 + w)
            return i;
        i += n;
    }
    return row.size();
}
