#pragma once
#include <editor/buffer_search.hpp>

#include <algorithm>
#include <cctype>
#include <regex>

namespace buffer_search_detail
{
    inline char lower_ch(char c)
    {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    inline bool is_word_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    inline bool whole_word_at(const std::string &line, std::size_t pos, std::size_t len)
    {
        if (len == 0)
            return false;
        const bool left_ok = (pos == 0) || !is_word_char(line[pos - 1]);
        const bool right_ok = (pos + len >= line.size()) || !is_word_char(line[pos + len]);
        return left_ok && right_ok;
    }

    inline bool cursor_leq(const Cursor &a, const Cursor &b)
    {
        return a.line < b.line || (a.line == b.line && a.column <= b.column);
    }

    inline bool match_starts_at_or_before(const BufferSearchMatch &m, Cursor from)
    {
        return cursor_leq(m.start, from);
    }
}
