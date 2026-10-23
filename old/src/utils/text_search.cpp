#include <utils/text_search.hpp>
#include <utils/async.hpp>
#include <utils/logger.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <format>
#include <regex>

bool TextSearch::should_skip_dir(const std::string &name)
{
    static const char *skip[] = {
        ".git",
        ".hg",
        ".svn",
        "node_modules",
        "target",
        "build",
        "dist",
        "out",
        ".cache",
        ".idea",
        ".vscode",
        ".cursor",
        "__pycache__",
        ".next",
        ".nuxt",
        "vendor",
        "CMakeFiles",
        ".tox",
        ".venv",
        "venv",
    };
    for (const char *s : skip)
    {
        if (name == s)
            return true;
    }
    return false;
}

bool TextSearch::looks_binary(const std::string &sample)
{
    const std::size_t n = std::min<std::size_t>(sample.size(), 512);
    for (std::size_t i = 0; i < n; ++i)
    {
        if (sample[i] == '\0')
            return true;
    }
    return false;
}

void TextSearch::set_root(const fs::path &root)
{
    std::lock_guard lock(mu);
    root_path = root;
}

void TextSearch::cancel()
{
    job_token.store(Background::instance().next_token(), std::memory_order_relaxed);
    searching.store(false, std::memory_order_release);
}

bool TextSearch::is_searching() const
{
    return searching.load(std::memory_order_acquire);
}

std::uint64_t TextSearch::version() const
{
    return ver.load(std::memory_order_acquire);
}

std::vector<TextMatch> TextSearch::results() const
{
    std::lock_guard lock(mu);
    return matches;
}

std::string TextSearch::status() const
{
    std::lock_guard lock(mu);
    return status_text;
}

