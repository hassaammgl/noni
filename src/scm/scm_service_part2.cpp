#include <scm/scm_service.hpp>
#include <scm/scm_git.hpp>
#include <utils/async.hpp>
#include <utils/git.hpp>
#include <utils/logger.hpp>

#include <format>
#include <system_error>

#include "scm_service_detail.hpp"

using namespace scm_service_detail;

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
