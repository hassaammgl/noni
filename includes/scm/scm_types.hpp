#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

// Document-relative SCM state — never screen/Window coordinates.
enum class ScmPathState : std::uint8_t
{
    Clean = 0,
    Modified,
    Added,
    Deleted,
    Renamed,
    Copied,
    Unmerged,
    Untracked,
    Ignored,
    Unknown,
};

struct ScmFileStatus
{
    fs::path path;           // absolute/canonical when available
    ScmPathState index = ScmPathState::Clean;    // staged (X)
    ScmPathState worktree = ScmPathState::Clean; // unstaged (Y)
    std::string rename_from; // when Renamed/Copied
    char xy[2]{' ', ' '};    // raw porcelain XY

    bool is_dirty() const
    {
        return index != ScmPathState::Clean || worktree != ScmPathState::Clean ||
               worktree == ScmPathState::Untracked || worktree == ScmPathState::Ignored;
    }
};

struct ScmRepoSnapshot
{
    fs::path root;
    bool is_repo = false;
    std::string branch = "[no git]";
    int ahead = -1;  // -1 = unknown / no upstream
    int behind = -1;
    std::unordered_map<std::string, ScmFileStatus> files; // key = normalized path string
    std::uint64_t generation = 0;
    std::string last_error;
};
