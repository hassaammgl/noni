#pragma once
#include <scm/scm_git.hpp>
#include <utils/logger.hpp>

#include <cctype>
#include <format>
#include <system_error>

namespace scm_git_detail
{
    inline std::string normalize_key(const fs::path &p)
    {
        std::error_code ec;
        fs::path abs = fs::weakly_canonical(p, ec);
        if (ec)
            abs = fs::absolute(p, ec);
        if (ec)
            abs = p;
        return abs.lexically_normal().string();
    }

    inline fs::path join_repo(const fs::path &root, const std::string &rel)
    {
        return (root / rel).lexically_normal();
    }
}
