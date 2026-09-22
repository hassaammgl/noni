#pragma once
#include <terminal/terminal_screen.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>

namespace terminal_screen_detail
{
    inline int glyph_width(char32_t cp)
    {
        if (cp == U'\t')
            return 1; // handled separately
        const int w = TextMetrics::codepoint_width(cp);
        if (w <= 0)
            return 1;
        return w > 2 ? 2 : w;
    }
}
