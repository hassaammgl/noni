#include <utils/text_metrics.hpp>

#include <cwchar>


namespace TextMetrics
{
    std::pair<char32_t, std::size_t> decode(std::string_view line, std::size_t byte)
    {
        if (byte >= line.size())
            return {0, 0};

        const unsigned char c0 = static_cast<unsigned char>(line[byte]);
        const std::size_t n = seq_len(c0);
        if (byte + n > line.size())
            return {static_cast<char32_t>(c0), 1};

        if (n == 1)
            return {c0, 1};

        char32_t cp = 0;
        if (n == 2)
        {
            const unsigned char c1 = static_cast<unsigned char>(line[byte + 1]);
            if (!is_cont(c1))
                return {c0, 1};
            cp = (static_cast<char32_t>(c0 & 0x1F) << 6) | (c1 & 0x3F);
        }
        else if (n == 3)
        {
            const unsigned char c1 = static_cast<unsigned char>(line[byte + 1]);
            const unsigned char c2 = static_cast<unsigned char>(line[byte + 2]);
            if (!is_cont(c1) || !is_cont(c2))
                return {c0, 1};
            cp = (static_cast<char32_t>(c0 & 0x0F) << 12) |
                 (static_cast<char32_t>(c1 & 0x3F) << 6) |
                 (c2 & 0x3F);
        }
        else
        {
            const unsigned char c1 = static_cast<unsigned char>(line[byte + 1]);
            const unsigned char c2 = static_cast<unsigned char>(line[byte + 2]);
            const unsigned char c3 = static_cast<unsigned char>(line[byte + 3]);
            if (!is_cont(c1) || !is_cont(c2) || !is_cont(c3))
                return {c0, 1};
            cp = (static_cast<char32_t>(c0 & 0x07) << 18) |
                 (static_cast<char32_t>(c1 & 0x3F) << 12) |
                 (static_cast<char32_t>(c2 & 0x3F) << 6) |
                 (c3 & 0x3F);
        }
        return {cp, n};
    }

    std::size_t snap_byte(std::string_view line, std::size_t byte)
    {
        if (byte >= line.size())
            return line.size();
        while (byte > 0 && is_cont(static_cast<unsigned char>(line[byte])))
            --byte;
        return byte;
    }

    std::size_t next_cp(std::string_view line, std::size_t byte)
    {
        if (byte >= line.size())
            return line.size();
        byte = snap_byte(line, byte);
        const auto [cp, n] = decode(line, byte);
        (void)cp;
        if (n == 0)
            return line.size();
        return std::min(line.size(), byte + n);
    }

    std::size_t prev_cp(std::string_view line, std::size_t byte)
    {
        if (byte == 0 || line.empty())
            return 0;
        if (byte > line.size())
            byte = line.size();
        --byte;
        return snap_byte(line, byte);
    }

    int codepoint_width(char32_t cp)
    {
        if (cp == U'\t')
            return -1;
        if (cp < 32 || (cp >= 0x7F && cp < 0xA0))
            return 1;
        const int w = ::wcwidth(static_cast<wchar_t>(cp));
        if (w < 0)
            return 1;
        return w;
    }

    int tab_width_at(int display_col, int tab_width)
    {
        if (tab_width <= 0)
            tab_width = kDefaultTabWidth;
        if (display_col < 0)
            display_col = 0;
        return tab_width - (display_col % tab_width);
    }

    int byte_to_display(std::string_view line, std::size_t byte, int tab_width)
    {
        if (byte > line.size())
            byte = line.size();
        byte = snap_byte(line, byte);

        int col = 0;
        std::size_t i = 0;
        while (i < byte)
        {
            const auto [cp, n] = decode(line, i);
            if (n == 0)
                break;
            if (cp == U'\t')
                col += tab_width_at(col, tab_width);
            else
                col += codepoint_width(cp);
            i += n;
        }
        return col;
    }

    std::size_t display_to_byte(std::string_view line, int display_col, int tab_width)
    {
        if (display_col <= 0)
            return 0;

        int col = 0;
        std::size_t i = 0;
        while (i < line.size())
        {
            const auto [cp, n] = decode(line, i);
            if (n == 0)
                break;
            int w = (cp == U'\t') ? tab_width_at(col, tab_width) : codepoint_width(cp);
            if (col + w > display_col)
                return i; // land on this codepoint
            col += w;
            i += n;
            if (col == display_col)
                return i;
        }
        return line.size();
    }

    int line_display_width(std::string_view line, int tab_width)
    {
        return byte_to_display(line, line.size(), tab_width);
    }

    int byte_to_codepoint_index(std::string_view line, std::size_t byte)
    {
        if (byte > line.size())
            byte = line.size();
        int idx = 0;
        std::size_t i = 0;
        while (i < byte)
        {
            const auto [cp, n] = decode(line, i);
            (void)cp;
            if (n == 0)
                break;
            i += n;
            ++idx;
        }
        return idx;
    }

}
