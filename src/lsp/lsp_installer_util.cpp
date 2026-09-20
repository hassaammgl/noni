#include "lsp_installer_detail.hpp"

#include <utils/logger.hpp>

#include <cstring>
#include <format>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

namespace lsp_install_detail
{
    const Spec *spec_for(const std::string &binary)
    {
        std::size_t n = 0;
        const Spec *specs = catalog_specs(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            if (binary == specs[i].binary)
                return &specs[i];
        }
        return nullptr;
    }

    const Spec *catalog_specs(std::size_t &count)
    {
        static const Spec specs[] = {
            {"pylsp", Method::PipVenv, "python-lsp-server", ""},
            {"typescript-language-server", Method::NpmPrefix, "typescript-language-server", "typescript"},
            {"clangd", Method::CurlGithubZip, "clangd/clangd", "19.1.2"},
            {"gopls", Method::GoInstall, "golang.org/x/tools/gopls", "latest"},
            {"rust-analyzer", Method::CurlGithubZip, "rust-lang/rust-analyzer", "2024-12-30"},
        };
        count = sizeof(specs) / sizeof(specs[0]);
        return specs;
    }

    bool exe_ok(const char *name)
    {
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
                    if (!ec && ((perms & fs::perms::owner_exec) != fs::perms::none ||
                                (perms & fs::perms::group_exec) != fs::perms::none ||
                                (perms & fs::perms::others_exec) != fs::perms::none))
                        return true;
                }
            }
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        return std::system(std::format("command -v {} >/dev/null 2>&1", name).c_str()) == 0;
    }

    int run_cmd(const std::string &cmd)
    {
        (void)mkdir("logs", 0755);
        const std::string wrapped = cmd + " >>logs/lsp-install.log 2>&1";
        Logger::debug(std::format("lsp-install: {}", cmd));
        return std::system(wrapped.c_str());
    }

    bool link_bin(const fs::path &target, const fs::path &link)
    {
        std::error_code ec;
        fs::create_directories(link.parent_path(), ec);
        fs::remove(link, ec);
        fs::create_symlink(target, link, ec);
        if (!ec && fs::exists(link))
            return true;
        fs::copy_file(target, link, fs::copy_options::overwrite_existing, ec);
        if (ec)
            return false;
        fs::permissions(link,
                        fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
                            fs::perms::others_read | fs::perms::others_exec,
                        ec);
        return fs::exists(link);
    }

    fs::path find_named_binary(const fs::path &root, const std::string &name)
    {
        std::error_code ec;
        if (!fs::exists(root, ec))
            return {};
        for (auto it = fs::recursive_directory_iterator(root, ec);
             !ec && it != fs::recursive_directory_iterator();
             it.increment(ec))
        {
            if (!it->is_regular_file(ec))
                continue;
            if (it->path().filename() == name)
                return it->path();
        }
        return {};
    }
}
