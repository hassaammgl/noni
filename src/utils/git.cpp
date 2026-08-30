#include <utils/git.hpp>
#include <utils/logger.hpp>
#include <cstdio>
#include <format>

Git::Git(fs::path repo_root) : repo_root(std::move(repo_root))
{
}

bool Git::is_repo() const
{
    return fs::exists(repo_root / ".git");
}

std::optional<std::string> Git::run_git(const std::string &args) const
{
    if (!is_repo())
        return std::nullopt;
    std::string cmd = std::format(
        "git -C {} {}",
        repo_root.string(),
        args);
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return std::nullopt;
    std::string out;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe))
        out += buf;
    pclose(pipe);
    // trim trailing newline
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        out.pop_back();
    return out;
}

std::optional<std::string> Git::current_branch() const
{
    return run_git("rev-parse --abbrev-ref HEAD");
}

std::vector<std::string> Git::status_porcelain() const
{
    std::vector<std::string> lines;
    auto out = run_git("status --porcelain");
    if (!out)
        return lines;
    return lines;
}

void Git::set_repo_root(const fs::path &root)
{
    repo_root = root;
}