#pragma once

#include <cstdint>
#include <string>
#include <vector>

// One contiguous document replacement, independent of ncurses.
struct TextChange
{
    int start_line = 0;
    int start_col = 0;
    std::string deleted;  // text removed (newlines as '\n')
    std::string inserted; // text inserted (newlines as '\n')
};

struct UndoTransaction
{
    std::vector<TextChange> changes;
    int cursor_before_line = 0;
    int cursor_before_col = 0;
    int cursor_after_line = 0;
    int cursor_after_col = 0;
    std::uint64_t id_before = 0;
    std::uint64_t id_after = 0;
};

struct UndoResult
{
    bool ok = false;
    int cursor_line = 0;
    int cursor_col = 0;
};
