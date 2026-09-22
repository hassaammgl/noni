#pragma once
#include <sidebar/dirscanner.hpp>
#include <utils/async.hpp>

#include <algorithm>
#include <format>
#include <ranges>

namespace dirscanner_detail
{
    inline bool should_skip(const fs::path &name)
    {
        static const char *skip[] = {
            ".git", ".hg", ".svn",
            "node_modules", "target", "build", "dist", "out",
            ".cache", ".idea", ".vscode", ".cursor",
            "__pycache__", ".next", ".nuxt", "vendor",
            "CMakeFiles", ".tox", ".venv", "venv",
        };
        const std::string n = name.string();
        for (const char *s : skip)
        {
            if (n == s)
                return true;
        }
        return false;
    }

    inline std::string path_key(const fs::path &path)
    {
        std::error_code ec;
        const fs::path abs = fs::weakly_canonical(path, ec);
        return ec ? path.lexically_normal().string() : abs.string();
    }
}
