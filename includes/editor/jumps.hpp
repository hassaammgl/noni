#pragma once

#include <utils/cursor.hpp>

#include <cstdint>
#include <vector>

struct JumpEntry
{
    std::uintptr_t buffer_id = 0;
    Cursor pos{.line = 0, .column = 0};
};

class JumpList
{
private:
    std::vector<JumpEntry> entries_;
    int index_ = -1; // points at current position in list
    static constexpr int kMax = 100;

public:
    void clear()
    {
        entries_.clear();
        index_ = -1;
    }

    void invalidate_buffer(std::uintptr_t buffer_id)
    {
        for (auto &e : entries_)
        {
            if (e.buffer_id == buffer_id)
                e.buffer_id = 0;
        }
    }

    // Record a jump from `from` before navigating to `to`.
    void push(std::uintptr_t buffer_id, Cursor from)
    {
        // Truncate forward history when jumping from middle.
        if (index_ + 1 < static_cast<int>(entries_.size()))
            entries_.erase(entries_.begin() + index_ + 1, entries_.end());

        JumpEntry entry{buffer_id, from};
        if (!entries_.empty())
        {
            const auto &last = entries_.back();
            if (last.buffer_id == buffer_id &&
                last.pos.line == from.line &&
                last.pos.column == from.column)
            {
                index_ = static_cast<int>(entries_.size()) - 1;
                return;
            }
        }

        entries_.push_back(entry);
        if (static_cast<int>(entries_.size()) > kMax)
            entries_.erase(entries_.begin());
        index_ = static_cast<int>(entries_.size()) - 1;
    }

    bool can_back() const { return index_ >= 0 && !entries_.empty(); }

    bool can_forward() const
    {
        return index_ + 1 < static_cast<int>(entries_.size());
    }

    // Before jumping back, optionally record current as forward target.
    const JumpEntry *back(std::uintptr_t current_buffer, Cursor current_pos)
    {
        if (!can_back())
            return nullptr;

        // Ensure current position is ahead for Ctrl-I.
        if (index_ + 1 == static_cast<int>(entries_.size()))
        {
            entries_.push_back(JumpEntry{current_buffer, current_pos});
            if (static_cast<int>(entries_.size()) > kMax)
            {
                entries_.erase(entries_.begin());
                --index_;
            }
        }
        else
        {
            entries_[static_cast<std::size_t>(index_ + 1)] =
                JumpEntry{current_buffer, current_pos};
        }

        const JumpEntry *entry = &entries_[static_cast<std::size_t>(index_)];
        --index_;
        if (entry->buffer_id == 0)
            return nullptr;
        return entry;
    }

    const JumpEntry *forward()
    {
        if (!can_forward())
            return nullptr;
        ++index_;
        const JumpEntry *entry = &entries_[static_cast<std::size_t>(index_)];
        if (entry->buffer_id == 0)
            return nullptr;
        return entry;
    }
};
