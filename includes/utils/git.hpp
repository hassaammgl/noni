#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Thin Git subprocess helper (no ncurses). Used by ScmGit backend.
class Git
{
private:
    fs::path repo_root_;

    std::optional<std::string> run_git(const std::vector<std::string> &args) const;

public:
    explicit Git(fs::path repo_root = fs::current_path());

    void set_repo_root(const fs::path &root);
    fs::path repo_root() const { return repo_root_; }

    bool is_repo() const;

    std::optional<std::string> current_branch() const;
    std::optional<std::string> status_porcelain_z() const;
    // Returns "ahead\tbehind" or nullopt.
    std::optional<std::string> ahead_behind() const;

    // Raw multiline stdout for custom commands.
    std::optional<std::string> run(const std::vector<std::string> &args) const;
};
