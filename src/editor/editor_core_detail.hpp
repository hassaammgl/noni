#pragma once
#include <editor/editor_core.hpp>
#include <utils/text_metrics.hpp>

namespace editor_core_detail
{
    inline void set_preferred_from_cursor(Window &w)
    {
        if (!w.has_buffer())
        {
            w.preferred_column() = 0;
            return;
        }
        const auto &lines = w.buffer().lines();
        const Cursor &c = w.cursor();
        if (lines.empty() || c.line < 0 || c.line >= static_cast<int>(lines.size()))
        {
            w.preferred_column() = 0;
            return;
        }
        w.preferred_column() = TextMetrics::byte_to_display(
            lines[static_cast<std::size_t>(c.line)], static_cast<std::size_t>(c.column));
    }
}
