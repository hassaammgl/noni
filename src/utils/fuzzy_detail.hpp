#pragma once
#include <utils/fuzzy.hpp>
#include <utils/async.hpp>
#include <utils/logger.hpp>

#include <algorithm>
#include <cctype>
#include <format>

namespace fuzzy_detail
{
    inline char lower(char c)
    {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    inline bool is_skipped_name(const std::string &name)
    {
        static const char *skip[] = {
            ".git", ".hg", ".svn", ".jj",
            "node_modules", "target", "build", "dist", "out",
            ".cache", ".idea", ".vscode", ".cursor",
            "__pycache__", ".next", ".nuxt", "vendor",
            "CMakeFiles", ".tox", ".venv", "venv",
        };
        for (const char *s : skip)
        {
            if (name == s)
                return true;
        }
        return false;
    }
}
