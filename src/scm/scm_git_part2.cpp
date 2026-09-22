#include <scm/scm_git.hpp>
#include <utils/logger.hpp>

#include <cctype>
#include <format>
#include <system_error>

#include "scm_git_detail.hpp"

using namespace scm_git_detail;

namespace ScmGit
{
    bool is_ignored(const fs::path &repo_root, const fs::path &path)
    {
        const std::string rel = repo_relative(repo_root, path);
        if (rel.empty())
            return false;
        Git git(repo_root);
        return git.run_ok({"check-ignore", "-q", "--", rel}, nullptr);
    }

    namespace
    {
        std::string first_line(std::string s)
        {
            while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
                s.pop_back();
            const auto nl = s.find('\n');
            if (nl != std::string::npos)
                s.resize(nl);
            while (!s.empty() && s.back() == '\r')
                s.pop_back();
            return s;
        }
    }

    std::optional<std::string> diff_vs_head(const fs::path &repo_root, const fs::path &path)
    {
        if (repo_root.empty() || path.empty())
            return std::nullopt;
        const std::string rel = repo_relative(repo_root, path);
        if (rel.empty())
            return std::nullopt;

        Git git(repo_root);
        if (!git.is_repo())
            return std::nullopt;

        std::string out;
        // Include staged + unstaged vs HEAD.
        if (!git.run_ok({"diff", "-U0", "HEAD", "--", rel}, &out))
        {
            // New file not in HEAD yet — try against empty tree / no HEAD.
            out.clear();
            if (!git.run_ok({"diff", "-U0", "--", rel}, &out))
                return std::string{};
        }
        return out;
    }

    bool stage_path(const fs::path &repo_root, const fs::path &path, std::string *error)
    {
        const std::string rel = repo_relative(repo_root, path);
        if (rel.empty())
        {
            if (error)
                *error = "path not in repository";
            return false;
        }
        if (is_ignored(repo_root, path))
        {
            if (error)
                *error = "file is gitignored";
            return false;
        }
        Git git(repo_root);
        std::string out;
        if (!git.run_ok({"add", "--", rel}, &out))
        {
            if (error)
                *error = out.empty() ? "git add failed" : first_line(std::move(out));
            return false;
        }
        return true;
    }

    bool unstage_path(const fs::path &repo_root, const fs::path &path, std::string *error)
    {
        const std::string rel = repo_relative(repo_root, path);
        if (rel.empty())
        {
            if (error)
                *error = "path not in repository";
            return false;
        }
        if (!is_tracked(repo_root, path))
        {
            if (error)
                *error = "file is not tracked — nothing to unstage";
            return false;
        }
        Git git(repo_root);
        std::string out;
        // Prefer restore --staged (modern); fall back to reset.
        if (!git.run_ok({"restore", "--staged", "--", rel}, &out))
        {
            out.clear();
            if (!git.run_ok({"reset", "HEAD", "--", rel}, &out))
            {
                if (error)
                    *error = out.empty() ? "git unstage failed" : first_line(std::move(out));
                return false;
            }
        }
        return true;
    }

    bool discard_path(const fs::path &repo_root, const fs::path &path, std::string *error)
    {
        const std::string rel = repo_relative(repo_root, path);
        if (rel.empty())
        {
            if (error)
                *error = "path not in repository";
            return false;
        }
        if (!is_tracked(repo_root, path))
        {
            if (error)
                *error = is_ignored(repo_root, path)
                             ? "file is gitignored — nothing to discard"
                             : "file is untracked — nothing to discard";
            return false;
        }
        Git git(repo_root);
        std::string out;
        if (!git.run_ok({"restore", "--worktree", "--", rel}, &out))
        {
            out.clear();
            if (!git.run_ok({"checkout", "--", rel}, &out))
            {
                if (error)
                    *error = out.empty() ? "git discard failed" : first_line(std::move(out));
                return false;
            }
        }
        return true;
    }
}
