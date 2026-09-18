#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

enum class ScmLineChange : std::uint8_t
{
    None = 0,
    Added,
    Modified,
    Deleted, // marker: lines removed at/near this buffer line
};

struct ScmHunk
{
    int old_start = 0; // 1-based
    int old_count = 0;
    int new_start = 0; // 1-based
    int new_count = 0;
};

// Line-level file diff vs a base (usually HEAD). Independent of ncurses.
struct ScmFileDiff
{
    fs::path path;
    std::vector<ScmHunk> hunks;
    // 0-based buffer line → change kind (Added/Modified/Deleted)
    std::unordered_map<int, ScmLineChange> lines;
    bool is_untracked = false;
    bool ok = false;
    std::string error;

    ScmLineChange line_change(int zero_based_line) const
    {
        auto it = lines.find(zero_based_line);
        return it == lines.end() ? ScmLineChange::None : it->second;
    }
};

namespace ScmDiff
{
    // Parse unified diff (-U0 preferred). `new_line_count` = current buffer lines.
    ScmFileDiff parse_unified(const std::string &diff_text, fs::path path, int new_line_count);

    // All lines marked Added (untracked new file).
    ScmFileDiff untracked_all_added(fs::path path, int new_line_count);
}
