#pragma once
#include <editor/buffer.hpp>
#include <utils/logger.hpp>
#include <utils/text_metrics.hpp>
#include <algorithm>
#include <format>
#include <system_error>

namespace buffer_detail
{
    inline void advance_pos(
        int &line,
        int &col,
        std::string_view text,
        const std::vector<std::string> & /*lines*/)
    {
        for (char ch : text)
        {
            if (ch == '\n')
            {
                ++line;
                col = 0;
            }
            else
            {
                ++col;
            }
        }
    }
}
