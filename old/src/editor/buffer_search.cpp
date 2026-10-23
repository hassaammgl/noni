#include <editor/buffer_search.hpp>

#include <algorithm>
#include <cctype>
#include <regex>

#include "buffer_search_detail.hpp"

using namespace buffer_search_detail;

std::optional<std::string> BufferSearch::validate(const BufferSearchQuery &query)
{
    if (query.pattern.empty())
        return std::string("Empty pattern");

    if (query.options.use_regex)
    {
        try
        {
            auto flags = std::regex::ECMAScript;
            if (!query.options.match_case)
                flags |= std::regex::icase;
            (void)std::regex(query.pattern, flags);
        }
        catch (const std::regex_error &e)
        {
            return std::string("Invalid regex: ") + e.what();
        }
        catch (...)
        {
            return std::string("Invalid regex");
        }
    }
    return std::nullopt;
}

std::vector<BufferSearchMatch> BufferSearch::find_all(
    const std::vector<std::string> &lines,
    const BufferSearchQuery &query)
{
    std::vector<BufferSearchMatch> out;
    if (query.pattern.empty() || lines.empty())
        return out;

    std::regex re;
    if (query.options.use_regex)
    {
        try
        {
            auto flags = std::regex::ECMAScript;
            if (!query.options.match_case)
                flags |= std::regex::icase;
            re = std::regex(query.pattern, flags);
        }
        catch (...)
        {
            return out;
        }
    }

    for (int line_i = 0; line_i < static_cast<int>(lines.size()); ++line_i)
    {
        const std::string &line = lines[static_cast<std::size_t>(line_i)];

        if (query.options.use_regex)
        {
            try
            {
                for (std::sregex_iterator ri(line.begin(), line.end(), re), rend;
                     ri != rend; ++ri)
                {
                    const auto len = static_cast<std::size_t>(ri->length());
                    const auto pos = static_cast<std::size_t>(ri->position());
                    if (query.options.whole_word && !whole_word_at(line, pos, len))
                        continue;
                    BufferSearchMatch m;
                    m.start = {.line = line_i, .column = static_cast<int>(pos)};
                    m.end = {.line = line_i, .column = static_cast<int>(pos + len)};
                    out.push_back(m);
                }
            }
            catch (...)
            {
            }
            continue;
        }

        const std::string hay = query.options.match_case ? line : [&]() {
            std::string s = line;
            for (char &c : s)
                c = lower_ch(c);
            return s;
        }();
        const std::string needle = query.options.match_case ? query.pattern : [&]() {
            std::string s = query.pattern;
            for (char &c : s)
                c = lower_ch(c);
            return s;
        }();

        if (needle.empty())
            continue;

        std::size_t pos = 0;
        while ((pos = hay.find(needle, pos)) != std::string::npos)
        {
            if (!query.options.whole_word || whole_word_at(line, pos, needle.size()))
            {
                BufferSearchMatch m;
                m.start = {.line = line_i, .column = static_cast<int>(pos)};
                m.end = {
                    .line = line_i,
                    .column = static_cast<int>(pos + needle.size())};
                out.push_back(m);
            }
            pos += std::max<std::size_t>(1, needle.size());
        }
    }

    return out;
}

