#include <utils/text_metrics.hpp>

#include <cwchar>


namespace TextMetrics
{
    std::size_t codepoint_index_to_byte(std::string_view line, int index)
    {
        if (index <= 0)
            return 0;
        int idx = 0;
        std::size_t i = 0;
        while (i < line.size() && idx < index)
        {
            const auto [cp, n] = decode(line, i);
            (void)cp;
            if (n == 0)
                break;
            i += n;
            ++idx;
        }
        return i;
    }

    int byte_to_utf16(std::string_view line, std::size_t byte)
    {
        if (byte > line.size())
            byte = line.size();
        byte = snap_byte(line, byte);
        int units = 0;
        std::size_t i = 0;
        while (i < byte)
        {
            const auto [cp, n] = decode(line, i);
            if (n == 0)
                break;
            units += (cp > 0xFFFF) ? 2 : 1;
            i += n;
        }
        return units;
    }

    std::size_t utf16_to_byte(std::string_view line, int utf16_col)
    {
        if (utf16_col <= 0)
            return 0;
        int units = 0;
        std::size_t i = 0;
        while (i < line.size())
        {
            const auto [cp, n] = decode(line, i);
            if (n == 0)
                break;
            const int w = (cp > 0xFFFF) ? 2 : 1;
            if (units + w > utf16_col)
                return i;
            units += w;
            i += n;
            if (units == utf16_col)
                return i;
        }
        return line.size();
    }
}
