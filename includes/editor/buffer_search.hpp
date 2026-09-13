#pragma once

#include <editor/selection.hpp>
#include <utils/cursor.hpp>
#include <utils/text_search.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class SearchDirection
{
    Forward,
    Backward,
};

// Buffer-relative match. No ncurses / screen coordinates.
struct BufferSearchMatch
{
    Cursor start{.line = 0, .column = 0};
    Cursor end{.line = 0, .column = 0}; // exclusive

    TextRange as_range() const
    {
        TextRange r;
        r.start = start;
        r.end = end;
        r.kind = SelectionKind::Character;
        return r;
    }
};

struct BufferSearchQuery
{
    std::string pattern;
    TextSearchOptions options{};
    SearchDirection direction = SearchDirection::Forward;
};

// Pure search over document lines — no UI, no Buffer mutation.
namespace BufferSearch
{
    // Empty on success; error message on invalid regex / empty pattern.
    std::optional<std::string> validate(const BufferSearchQuery &query);

    std::vector<BufferSearchMatch> find_all(
        const std::vector<std::string> &lines,
        const BufferSearchQuery &query);

    // Index of first match at/after (forward) or at/before (backward) cursor.
    // Returns -1 if none. wrap selects wraparound start index.
    int find_next_index(
        const std::vector<BufferSearchMatch> &matches,
        Cursor from,
        SearchDirection dir,
        bool wrap);
}

// Session state owned by EditorCore (not Buffer, not UI).
class BufferSearchState
{
private:
    BufferSearchQuery query_{};
    std::vector<BufferSearchMatch> matches_;
    int current_ = -1;
    std::uintptr_t buffer_id_ = 0;
    std::uint64_t buffer_revision_ = 0;
    bool active_ = false;
    std::string last_error_;

public:
    void clear()
    {
        matches_.clear();
        current_ = -1;
        buffer_id_ = 0;
        buffer_revision_ = 0;
        active_ = false;
        last_error_.clear();
    }

    bool active() const { return active_; }
    const BufferSearchQuery &query() const { return query_; }
    const std::vector<BufferSearchMatch> &matches() const { return matches_; }
    int current_index() const { return current_; }
    const std::string &last_error() const { return last_error_; }
    std::uintptr_t buffer_id() const { return buffer_id_; }

    const BufferSearchMatch *current_match() const
    {
        if (current_ < 0 || current_ >= static_cast<int>(matches_.size()))
            return nullptr;
        return &matches_[static_cast<std::size_t>(current_)];
    }

    void bind_buffer(std::uintptr_t id, std::uint64_t revision)
    {
        buffer_id_ = id;
        buffer_revision_ = revision;
    }

    bool stale(std::uintptr_t id, std::uint64_t revision) const
    {
        return !active_ || buffer_id_ != id || buffer_revision_ != revision;
    }

    // Run search; sets current to first match from `from` in query.direction.
    // Returns false on error/no match (last_error_ set on error).
    bool run(
        const std::vector<std::string> &lines,
        std::uintptr_t buffer_id,
        std::uint64_t revision,
        BufferSearchQuery query,
        Cursor from);

    // Refresh matches if revision changed; keeps relative current when possible.
    bool refresh_if_needed(
        const std::vector<std::string> &lines,
        std::uintptr_t buffer_id,
        std::uint64_t revision);

    // Move current index; wraparound. Returns false if no matches.
    bool step(bool reverse);

    void set_current_index(int idx)
    {
        if (idx >= 0 && idx < static_cast<int>(matches_.size()))
            current_ = idx;
    }
};
