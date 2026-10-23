#include <utils/git.hpp>

#include <array>
#include <cstdio>
#include <sstream>
#include <system_error>

namespace
{
    std::string shell_quote(const std::string &s)
    {
        std::string out = "'";
        for (char c : s)
        {
            if (c == '\'')
                out += "'\\''";
            else
                out += c;
        }
        out += "'";
        return out;
    }
}

Git::Git(fs::path repo_root) : repo_root_(std::move(repo_root))
{
}

void Git::set_repo_root(const fs::path &root)
{
    repo_root_ = root;
}

bool Git::is_repo() const
{
    if (repo_root_.empty())
        return false;
    std::error_code ec;
    return fs::exists(repo_root_ / ".git", ec);
}

std::optional<std::string> Git::run_git(const std::vector<std::string> &args) const
{
    return run(args);
}

std::optional<std::string> Git::run(const std::vector<std::string> &args) const
{
    std::string out;
    if (!run_ok(args, &out))
    {
        if (out.empty())
            return std::nullopt;
    }
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        out.pop_back();
    return out;
}

bool Git::run_ok(const std::vector<std::string> &args, std::string *stdout_out) const
{
    if (repo_root_.empty())
        return false;

    // Capture stderr too — never leak git chatter onto the ncurses TTY.
    std::ostringstream cmd;
    cmd << "git -C " << shell_quote(repo_root_.string());
    for (const auto &a : args)
        cmd << ' ' << shell_quote(a);
    cmd << " 2>&1";

    FILE *pipe = popen(cmd.str().c_str(), "r");
    if (!pipe)
        return false;

    std::string out;
    std::array<char, 512> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        out += buf.data();

    const int rc = pclose(pipe);
    if (stdout_out)
        *stdout_out = std::move(out);
    return rc == 0;
}

std::optional<std::string> Git::current_branch() const
{
    if (!is_repo())
        return std::nullopt;
    return run({"rev-parse", "--abbrev-ref", "HEAD"});
}

std::optional<std::string> Git::status_porcelain_z() const
{
    if (!is_repo())
        return std::nullopt;
    // Keep trailing NULs; do not trim.
    std::ostringstream cmd;
    cmd << "git -C " << shell_quote(repo_root_.string())
        << " status --porcelain -z 2>/dev/null";
    FILE *pipe = popen(cmd.str().c_str(), "r");
    if (!pipe)
        return std::nullopt;
    std::string out;
    std::array<char, 512> buf{};
    while (true)
    {
        const size_t n = fread(buf.data(), 1, buf.size(), pipe);
        if (n == 0)
            break;
        out.append(buf.data(), n);
    }
    pclose(pipe);
    return out;
}

std::optional<std::string> Git::ahead_behind() const
{
    if (!is_repo())
        return std::nullopt;
    // "A\tB" left=behind upstream, right=ahead of upstream for @{upstream}...HEAD
    // rev-list --left-right --count A...B : left = commits reachable from A not B
    auto out = run({"rev-list", "--left-right", "--count", "@{upstream}...HEAD"});
    return out;
}
