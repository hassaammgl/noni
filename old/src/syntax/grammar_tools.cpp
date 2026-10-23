#include "grammar_state.hpp"
#include <utils/logger.hpp>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <sys/stat.h>
#include <sys/types.h>
namespace grammar_install_state
{
    bool exe_ok(const char *name)
    {
        // PATH lookup without spawning a shell login profile.
        const char *path = std::getenv("PATH");
        if (!path || !*path)
            return false;
        std::string paths = path;
        std::size_t start = 0;
        while (start <= paths.size())
        {
            const std::size_t end = paths.find(':', start);
            const std::string dir =
                paths.substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (!dir.empty())
            {
                const fs::path cand = fs::path(dir) / name;
                std::error_code ec;
                if (fs::is_regular_file(cand, ec) && !ec)
                {
                    const auto perms = fs::status(cand, ec).permissions();
                    if (!ec && (perms & fs::perms::owner_exec) != fs::perms::none)
                        return true;
                    // Also accept if any execute bit is set.
                    if (!ec && ((perms & fs::perms::group_exec) != fs::perms::none ||
                                (perms & fs::perms::others_exec) != fs::perms::none))
                        return true;
                }
            }
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        // Fallback: let the shell resolve (handles wrappers / different permission models).
        return std::system(std::format("command -v {} >/dev/null 2>&1", name).c_str()) == 0;
    }

    int run_cmd(const std::string &cmd)
    {
        // Keep install chatter out of the TTY (same class of bug as LSP stderr).
        (void)mkdir("logs", 0755);
        const std::string wrapped = cmd + " >>logs/grammar-install.log 2>&1";
        Logger::debug(std::format("grammar-install: {}", cmd));
        return std::system(wrapped.c_str());
    }

    std::string repo_basename(const std::string &url)
    {
        std::string name = url;
        while (!name.empty() && name.back() == '/')
            name.pop_back();
        if (name.size() > 4 && name.substr(name.size() - 4) == ".git")
            name = name.substr(0, name.size() - 4);
        const auto pos = name.find_last_of('/');
        if (pos != std::string::npos)
            name = name.substr(pos + 1);
        return name;
    }
}
