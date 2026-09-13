#pragma once

#include <scm/scm_types.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>

namespace fs = std::filesystem;

// Editor-owned SCM cache. Refresh is async; UI reads snapshots only.
class ScmService
{
public:
    ScmService() = default;

    // Workspace / project root hint (not Window-dependent).
    void set_workspace_root(const fs::path &root);
    fs::path workspace_root() const;

    // Schedule background refresh. Cheap to call; coalesces via token.
    void request_refresh();

    // Immediate sync refresh (tests / rare paths). Prefer request_refresh.
    void refresh_now();

    ScmRepoSnapshot snapshot() const;
    std::uint64_t generation() const;

    std::optional<ScmFileStatus> status_for(const fs::path &path) const;
    std::string branch() const;

private:
    void apply_snapshot(ScmRepoSnapshot snap, std::uint64_t token);

    mutable std::mutex mu_;
    fs::path workspace_root_;
    ScmRepoSnapshot snapshot_;
    std::atomic<std::uint64_t> refresh_token_{0};
};
