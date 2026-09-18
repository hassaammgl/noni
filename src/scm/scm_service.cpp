#include <scm/scm_service.hpp>
#include <scm/scm_git.hpp>
#include <utils/async.hpp>
#include <utils/git.hpp>
#include <utils/logger.hpp>

#include <format>
#include <system_error>

namespace
{
    std::string normalize_key(const fs::path &p)
    {
        std::error_code ec;
        fs::path abs = fs::weakly_canonical(p, ec);
        if (ec)
            abs = fs::absolute(p, ec);
        if (ec)
            abs = p;
        return abs.lexically_normal().string();
    }
}

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

std::uint64_t ScmService::diff_generation() const
{
    std::lock_guard lock(mu_);
    return diff_generation_;
}

bool ScmService::stage(const fs::path &path, std::string *error)
{
    fs::path root;
    {
        std::lock_guard lock(mu_);
        root = snapshot_.root.empty() ? ScmGit::find_repository_root(workspace_root_)
                                      : snapshot_.root;
    }
    if (root.empty())
    {
        if (error)
            *error = "not a git repository";
        return false;
    }
    if (!ScmGit::stage_path(root, path, error))
        return false;
    request_refresh();
    return true;
}

bool ScmService::unstage(const fs::path &path, std::string *error)
{
    fs::path root;
    {
        std::lock_guard lock(mu_);
        root = snapshot_.root.empty() ? ScmGit::find_repository_root(workspace_root_)
                                      : snapshot_.root;
    }
    if (root.empty())
    {
        if (error)
            *error = "not a git repository";
        return false;
    }
    if (!ScmGit::unstage_path(root, path, error))
        return false;
    request_refresh();
    return true;
}

bool ScmService::discard_worktree(const fs::path &path, std::string *error)
{
    fs::path root;
    {
        std::lock_guard lock(mu_);
        root = snapshot_.root.empty() ? ScmGit::find_repository_root(workspace_root_)
                                      : snapshot_.root;
    }
    if (root.empty())
    {
        if (error)
            *error = "not a git repository";
        return false;
    }
    if (!ScmGit::discard_path(root, path, error))
        return false;
    request_refresh();
    return true;
}

ScmRepoSnapshot ScmService::snapshot() const
{
    std::lock_guard lock(mu_);
    return snapshot_;
}

std::uint64_t ScmService::generation() const
{
    std::lock_guard lock(mu_);
    return snapshot_.generation;
}

std::string ScmService::branch() const
{
    std::lock_guard lock(mu_);
    return snapshot_.branch;
}

std::optional<ScmFileStatus> ScmService::status_for(const fs::path &path) const
{
    if (path.empty())
        return std::nullopt;

    const std::string key = normalize_key(path);
    std::lock_guard lock(mu_);
    auto it = snapshot_.files.find(key);
    if (it == snapshot_.files.end())
    {
        if (!snapshot_.root.empty())
        {
            std::error_code ec;
            fs::path rel = fs::relative(path, snapshot_.root, ec);
            if (!ec)
            {
                const std::string alt = normalize_key(snapshot_.root / rel);
                it = snapshot_.files.find(alt);
            }
        }
    }
    if (it == snapshot_.files.end())
        return std::nullopt;
    return it->second;
}
