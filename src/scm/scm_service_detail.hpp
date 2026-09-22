#pragma once
#include <scm/scm_service.hpp>
#include <scm/scm_git.hpp>
#include <utils/async.hpp>
#include <utils/git.hpp>
#include <utils/logger.hpp>

#include <format>
#include <system_error>

namespace scm_service_detail
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
}
