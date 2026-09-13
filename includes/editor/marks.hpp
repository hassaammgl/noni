#pragma once

#include <utils/cursor.hpp>

#include <array>
#include <cstdint>

struct Mark
{
    bool valid = false;
    std::uintptr_t buffer_id = 0;
    Cursor pos{.line = 0, .column = 0};
};

// Local marks a-z. Stored as editor/application state (not Buffer).
class MarkTable
{
private:
    std::array<Mark, 26> marks_{};

    static int index_of(char name)
    {
        if (name >= 'a' && name <= 'z')
            return name - 'a';
        return -1;
    }

public:
    void set(char name, std::uintptr_t buffer_id, Cursor pos)
    {
        const int i = index_of(name);
        if (i < 0)
            return;
        marks_[static_cast<std::size_t>(i)] = Mark{true, buffer_id, pos};
    }

    const Mark *get(char name) const
    {
        const int i = index_of(name);
        if (i < 0)
            return nullptr;
        const Mark &m = marks_[static_cast<std::size_t>(i)];
        return m.valid ? &m : nullptr;
    }

    void invalidate_buffer(std::uintptr_t buffer_id)
    {
        for (auto &m : marks_)
        {
            if (m.valid && m.buffer_id == buffer_id)
                m.valid = false;
        }
    }
};
