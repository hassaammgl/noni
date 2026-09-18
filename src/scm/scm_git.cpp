#include <scm/scm_git.hpp>
#include <utils/logger.hpp>

#include <cctype>
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

    fs::path join_repo(const fs::path &root, const std::string &rel)
    {
        return (root / rel).lexically_normal();
    }
}

namespace ScmGit
{
    fs::path find_repository_root(const fs::path &hint)
    {
        std::error_code ec;
        fs::path cur = hint.empty() ? fs::current_path(ec) : hint;
        if (ec)
            return {};
        if (!fs::is_directory(cur, ec))
            cur = cur.parent_path();

        while (!cur.empty())
        {
            if (fs::exists(cur / ".git", ec))
                return cur;
            fs::path parent = cur.parent_path();
            if (parent == cur)
                break;
            cur = parent;
        }
        return {};
    }

    ScmPathState decode_status_char(char c)
    {
        switch (c)
        {
        case ' ':
            return ScmPathState::Clean;
        case 'M':
            return ScmPathState::Modified;
        case 'A':
            return ScmPathState::Added;
        case 'D':
            return ScmPathState::Deleted;
        case 'R':
            return ScmPathState::Renamed;
        case 'C':
            return ScmPathState::Copied;
        case 'U':
            return ScmPathState::Unmerged;
        case '?':
            return ScmPathState::Untracked;
        case '!':
            return ScmPathState::Ignored;
        default:
            return ScmPathState::Unknown;
        }
    }

    ScmRepoSnapshot collect(const fs::path &repo_root)
    {
        ScmRepoSnapshot snap;
        snap.root = repo_root;
        if (repo_root.empty())
        {
            snap.branch = "[no git]";
            return snap;
        }

        Git git(repo_root);
        if (!git.is_repo())
        {
            snap.branch = "[no git]";
            return snap;
        }

        snap.is_repo = true;
        if (auto b = git.current_branch())
            snap.branch = *b;
        else
            snap.branch = "HEAD";

        if (auto ab = git.ahead_behind())
        {
            // "behind\tahead" from --left-right upstream...HEAD
            const auto &s = *ab;
            const auto tab = s.find('\t');
            if (tab != std::string::npos)
            {
                try
                {
                    snap.behind = std::stoi(s.substr(0, tab));
                    snap.ahead = std::stoi(s.substr(tab + 1));
                }
                catch (...)
                {
                    snap.ahead = -1;
                    snap.behind = -1;
                }
            }
        }

        auto porcelain = git.status_porcelain_z();
        if (!porcelain)
            return snap;

        const std::string &raw = *porcelain;
        std::size_t i = 0;
        while (i < raw.size())
        {
            if (i + 3 > raw.size())
                break;
            const char x = raw[i];
            const char y = raw[i + 1];
            // Format: XY<space>path\0  or XY<path for untracked still has space
            std::size_t path_start = i + 2;
            if (path_start < raw.size() && raw[path_start] == ' ')
                ++path_start;

            std::size_t nul = raw.find('\0', path_start);
            if (nul == std::string::npos)
                break;
            std::string path1 = raw.substr(path_start, nul - path_start);
            i = nul + 1;

            ScmFileStatus st;
            st.xy[0] = x;
            st.xy[1] = y;
            st.index = decode_status_char(x);
            st.worktree = decode_status_char(y);

            if ((x == 'R' || x == 'C' || y == 'R' || y == 'C') && i < raw.size())
            {
                nul = raw.find('\0', i);
                if (nul == std::string::npos)
                    break;
                std::string path2 = raw.substr(i, nul - i);
                i = nul + 1;
                st.path = join_repo(repo_root, path1);
                st.rename_from = path2;
            }
            else
            {
                st.path = join_repo(repo_root, path1);
            }

            snap.files[normalize_key(st.path)] = std::move(st);
        }

        return snap;
    }

    std::string repo_relative(const fs::path &repo_root, const fs::path &path)
    {
        if (repo_root.empty() || path.empty())
            return {};
        std::error_code ec;
        fs::path abs = fs::weakly_canonical(path, ec);
        if (ec)
            abs = fs::absolute(path, ec);
        if (ec)
            abs = path;
        fs::path rel = fs::relative(abs, repo_root, ec);
        if (ec || rel.empty() || *rel.begin() == "..")
            return {};
        return rel.generic_string();
    }

    bool is_tracked(const fs::path &repo_root, const fs::path &path)
    {
        const std::string rel = repo_relative(repo_root, path);
        if (rel.empty())
            return false;
        Git git(repo_root);
        std::string out;
        if (!git.run_ok({"ls-files", "--", rel}, &out))
            return false;
        while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
            out.pop_back();
        return !out.empty();
    }

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
