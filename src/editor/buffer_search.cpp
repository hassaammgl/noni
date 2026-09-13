#include <editor/buffer_search.hpp>

#include <algorithm>
#include <cctype>
#include <regex>

namespace
{
    char lower_ch(char c)
    {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    bool is_word_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    bool whole_word_at(const std::string &line, std::size_t pos, std::size_t len)
    {
        if (len == 0)
            return false;
        const bool left_ok = (pos == 0) || !is_word_char(line[pos - 1]);
        const bool right_ok = (pos + len >= line.size()) || !is_word_char(line[pos + len]);
        return left_ok && right_ok;
    }

    bool cursor_leq(const Cursor &a, const Cursor &b)
    {
        return a.line < b.line || (a.line == b.line && a.column <= b.column);
    }

    bool match_starts_at_or_before(const BufferSearchMatch &m, Cursor from)
    {
        return cursor_leq(m.start, from);
    }
}

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

int BufferSearch::find_next_index(
    const std::vector<BufferSearchMatch> &matches,
    Cursor from,
    SearchDirection dir,
    bool wrap)
{
    if (matches.empty())
        return -1;

    if (dir == SearchDirection::Forward)
    {
        // First match with start >= from
        for (int i = 0; i < static_cast<int>(matches.size()); ++i)
        {
            if (cursor_leq(from, matches[static_cast<std::size_t>(i)].start))
                return i;
        }
        return wrap ? 0 : -1;
    }

    // Backward: last match with start <= from
    for (int i = static_cast<int>(matches.size()) - 1; i >= 0; --i)
    {
        if (match_starts_at_or_before(matches[static_cast<std::size_t>(i)], from))
            return i;
    }
    return wrap ? static_cast<int>(matches.size()) - 1 : -1;
}

bool BufferSearchState::run(
    const std::vector<std::string> &lines,
    std::uintptr_t buffer_id,
    std::uint64_t revision,
    BufferSearchQuery query,
    Cursor from)
{
    last_error_.clear();
    if (auto err = BufferSearch::validate(query))
    {
        clear();
        last_error_ = *err;
        return false;
    }

    // Smart-case: uppercase in pattern → case-sensitive.
    if (!query.options.match_case)
    {
        for (char c : query.pattern)
        {
            if (std::isupper(static_cast<unsigned char>(c)))
            {
                query.options.match_case = true;
                break;
            }
        }
    }

    query_ = std::move(query);
    matches_ = BufferSearch::find_all(lines, query_);
    buffer_id_ = buffer_id;
    buffer_revision_ = revision;
    active_ = true;

    if (matches_.empty())
    {
        current_ = -1;
        last_error_ = "Pattern not found";
        return false;
    }

    current_ = BufferSearch::find_next_index(matches_, from, query_.direction, true);
    if (current_ < 0)
        current_ = 0;
    return true;
}

bool BufferSearchState::refresh_if_needed(
    const std::vector<std::string> &lines,
    std::uintptr_t buffer_id,
    std::uint64_t revision)
{
    if (!active_)
        return false;
    if (buffer_id_ != buffer_id)
    {
        clear();
        return false;
    }
    if (buffer_revision_ == revision)
        return true;

    Cursor anchor{.line = 0, .column = 0};
    if (current_match())
        anchor = current_match()->start;

    matches_ = BufferSearch::find_all(lines, query_);
    buffer_revision_ = revision;
    if (matches_.empty())
    {
        current_ = -1;
        return false;
    }
    current_ = BufferSearch::find_next_index(matches_, anchor, SearchDirection::Forward, true);
    if (current_ < 0)
        current_ = 0;
    return true;
}

bool BufferSearchState::step(bool reverse)
{
    if (matches_.empty())
        return false;

    const bool go_forward =
        (query_.direction == SearchDirection::Forward) ? !reverse : reverse;

    if (go_forward)
    {
        current_ = (current_ + 1) % static_cast<int>(matches_.size());
    }
    else
    {
        current_ = (current_ - 1 + static_cast<int>(matches_.size())) %
                   static_cast<int>(matches_.size());
    }
    return true;
}
