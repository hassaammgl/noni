#pragma once

#include <scm/scm_diff.hpp>
#include <scm/scm_types.hpp>
#include <utils/git.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Git subprocess backend for SCM (no ncurses). Reuses utils/Git.
namespace ScmGit
{
    fs::path find_repository_root(const fs::path &hint);
    ScmRepoSnapshot collect(const fs::path &repo_root);
    ScmPathState decode_status_char(char c);

    std::string repo_relative(const fs::path &repo_root, const fs::path &path);

    // Quiet checks (no pathspec stderr).
    bool is_tracked(const fs::path &repo_root, const fs::path &path);
    bool is_ignored(const fs::path &repo_root, const fs::path &path);

    // Unified diff vs HEAD (-U0). nullopt = git failure; empty = clean.
    std::optional<std::string> diff_vs_head(const fs::path &repo_root, const fs::path &path);

    bool stage_path(const fs::path &repo_root, const fs::path &path, std::string *error = nullptr);
    bool unstage_path(const fs::path &repo_root, const fs::path &path, std::string *error = nullptr);
    bool discard_path(const fs::path &repo_root, const fs::path &path, std::string *error = nullptr);
}
