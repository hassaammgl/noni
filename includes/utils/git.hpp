#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class Git
{
private:
    fs::path repo_root;
    std::optional<std::string>
    run_git(const std::string &args) const;

public:
    explicit Git(fs::path repo_root = fs::current_path());
    bool is_repo() const;
    std::optional<std::string> current_branch() const;
    std::vector<std::string> status_porcelain() const;
    void set_repo_root(const fs::path &root);
};