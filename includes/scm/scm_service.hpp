#pragma once

#include <scm/scm_diff.hpp>
#include <scm/scm_types.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

// Editor-owned SCM cache. Refresh is async; UI reads snapshots only.
class ScmService
{
public:
    ScmService() = default;

    void set_workspace_root(const fs::path &root);
    fs::path workspace_root() const;

    void request_refresh();
    void refresh_now();

    ScmRepoSnapshot snapshot() const;
    std::uint64_t generation() const;

    std::optional<ScmFileStatus> status_for(const fs::path &path) const;
    std::string branch() const;

    // Async line-diff vs HEAD for gutter. Coalesced per path.
    void request_file_diff(const fs::path &path, int buffer_line_count);
    std::optional<ScmFileDiff> file_diff(const fs::path &path) const;
    std::uint64_t diff_generation() const;

    // Sync mutations (call from command handlers). Refresh status after.
    bool stage(const fs::path &path, std::string *error = nullptr);
    bool unstage(const fs::path &path, std::string *error = nullptr);
    bool discard_worktree(const fs::path &path, std::string *error = nullptr);

private:
    void apply_snapshot(ScmRepoSnapshot snap, std::uint64_t token);
    void apply_file_diff(fs::path path, ScmFileDiff diff, std::uint64_t token);

    mutable std::mutex mu_;
    fs::path workspace_root_;
    ScmRepoSnapshot snapshot_;
    std::atomic<std::uint64_t> refresh_token_{0};

    std::unordered_map<std::string, ScmFileDiff> diffs_;
    std::atomic<std::uint64_t> diff_token_{0};
    std::uint64_t diff_generation_ = 0;
};
