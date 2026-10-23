#include <scm/scm_service.hpp>
#include <scm/scm_git.hpp>
#include <utils/async.hpp>
#include <utils/git.hpp>
#include <utils/logger.hpp>

#include <format>
#include <system_error>

#include "scm_service_detail.hpp"

using namespace scm_service_detail;

void ScmService::set_workspace_root(const fs::path &root)
{
    bool changed = false;
    {
        std::lock_guard lock(mu_);
        if (workspace_root_ != root)
        {
            workspace_root_ = root;
            diffs_.clear();
            ++diff_generation_;
            changed = true;
        }
    }
    if (changed)
        request_refresh();
}

fs::path ScmService::workspace_root() const
{
    std::lock_guard lock(mu_);
    return workspace_root_;
}

void ScmService::apply_snapshot(ScmRepoSnapshot snap, std::uint64_t token)
{
    if (refresh_token_.load(std::memory_order_relaxed) != token)
        return;

    std::lock_guard lock(mu_);
    if (refresh_token_.load(std::memory_order_relaxed) != token)
        return;

    snap.generation = snapshot_.generation + 1;
    snapshot_ = std::move(snap);
}

void ScmService::apply_file_diff(fs::path path, ScmFileDiff diff, std::uint64_t token)
{
    if (diff_token_.load(std::memory_order_relaxed) != token)
        return;
    std::lock_guard lock(mu_);
    if (diff_token_.load(std::memory_order_relaxed) != token)
        return;
    diffs_[normalize_key(path)] = std::move(diff);
    ++diff_generation_;
}

void ScmService::refresh_now()
{
    fs::path hint;
    {
        std::lock_guard lock(mu_);
        hint = workspace_root_;
    }

    const fs::path root = ScmGit::find_repository_root(hint);
    ScmRepoSnapshot snap = ScmGit::collect(root);
    if (root.empty() && !hint.empty())
        snap.last_error = "not a git repository";

    const std::uint64_t token = refresh_token_.load(std::memory_order_relaxed);
    apply_snapshot(std::move(snap), token);
}

void ScmService::request_refresh()
{
    const std::uint64_t token = Background::instance().next_token();
    refresh_token_.store(token, std::memory_order_relaxed);

    fs::path hint;
    {
        std::lock_guard lock(mu_);
        hint = workspace_root_;
    }

    Background::instance().post([this, hint, token]() {
        if (refresh_token_.load(std::memory_order_relaxed) != token)
            return;

        const fs::path root = ScmGit::find_repository_root(hint);
        ScmRepoSnapshot snap = ScmGit::collect(root);
        if (root.empty())
        {
            snap.branch = "[no git]";
            snap.last_error = hint.empty() ? "no workspace" : "not a git repository";
        }

        apply_snapshot(std::move(snap), token);
    });
}

void ScmService::request_file_diff(const fs::path &path, int buffer_line_count)
{
    if (path.empty())
        return;

    const std::uint64_t token = Background::instance().next_token();
    diff_token_.store(token, std::memory_order_relaxed);

    fs::path hint;
    {
        std::lock_guard lock(mu_);
        hint = workspace_root_;
    }

    const fs::path path_copy = path;
    const int lines = buffer_line_count;

    Background::instance().post([this, hint, path_copy, lines, token]() {
        if (diff_token_.load(std::memory_order_relaxed) != token)
            return;

        const fs::path root = ScmGit::find_repository_root(hint.empty() ? path_copy : hint);
        ScmFileDiff diff;
        diff.path = path_copy;

        if (root.empty())
        {
            diff.ok = false;
            diff.error = "not a git repository";
            apply_file_diff(path_copy, std::move(diff), token);
            return;
        }

        // Quiet checks — never ls-files --error-unmatch (pathspec stderr spam).
        if (ScmGit::is_tracked(root, path_copy))
        {
            auto raw = ScmGit::diff_vs_head(root, path_copy);
            if (!raw)
            {
                diff.ok = false;
                diff.error = "diff failed";
                apply_file_diff(path_copy, std::move(diff), token);
                return;
            }
            diff = ScmDiff::parse_unified(*raw, path_copy, lines);
            apply_file_diff(path_copy, std::move(diff), token);
            return;
        }
        if (ScmGit::is_ignored(root, path_copy))
        {
            diff.ok = true;
            apply_file_diff(path_copy, std::move(diff), token);
            return;
        }

        diff = ScmDiff::untracked_all_added(path_copy, lines);
        apply_file_diff(path_copy, std::move(diff), token);
    });
}

std::optional<ScmFileDiff> ScmService::file_diff(const fs::path &path) const
{
    if (path.empty())
        return std::nullopt;
    std::lock_guard lock(mu_);
    auto it = diffs_.find(normalize_key(path));
    if (it == diffs_.end())
        return std::nullopt;
    return it->second;
}

