#pragma once

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
    // Walk parents for .git; empty if none.
    fs::path find_repository_root(const fs::path &hint);

    // Populate snapshot from a repository root (sync; call off UI thread).
    ScmRepoSnapshot collect(const fs::path &repo_root);

    ScmPathState decode_status_char(char c);
}
