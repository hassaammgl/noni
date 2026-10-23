#pragma once
#include <components/sidebar.hpp>
#include <ui/icons.hpp>
#include <ui/theme.hpp>
#include <algorithm>
#include <format>

namespace sidebar_detail
{
    inline std::string path_key(const fs::path &path)
    {
        std::error_code ec;
        const fs::path abs = fs::weakly_canonical(path, ec);
        return ec ? path.lexically_normal().string() : abs.string();
    }

    inline bool same_path(const fs::path &a, const fs::path &b)
    {
        if (a.empty() || b.empty())
            return false;
        return path_key(a) == path_key(b);
    }
}
