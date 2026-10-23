#include <editor/buffer_search.hpp>

#include <algorithm>
#include <cctype>
#include <regex>

#include "buffer_search_detail.hpp"

using namespace buffer_search_detail;

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
