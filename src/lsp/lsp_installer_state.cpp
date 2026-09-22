#include "lsp_installer_state.hpp"
#include <cstdio>
#include <cstdlib>
#include <format>
#include <unistd.h>
namespace lsp_installer_state
{
    std::mutex g_mu;
    bool g_auto_install = true;
    std::atomic<std::uint64_t> g_generation{1};
    std::unordered_map<std::string, LspInstaller::Status> g_status;
    std::unordered_set<std::string> g_queued;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> g_failed_at;
    fs::path g_workspace_root;
    fs::path g_user_dir_cache;
    bool g_user_dir_resolved = false;

    bool dir_writable(const fs::path &dir)
    {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec)
            return false;
        const fs::path probe = dir / ".write_test";
        std::FILE *f = std::fopen(probe.c_str(), "w");
        if (!f)
            return false;
        std::fclose(f);
        fs::remove(probe, ec);
        return true;
    }

    fs::path xdg_lsp_dir()
    {
        if (const char *home = std::getenv("HOME"); home && *home)
            return fs::path(home) / ".local" / "share" / "noni" / "lsp";
        return fs::path("/tmp/noni-lsp");
    }

    fs::path workspace_lsp_dir()
    {
        fs::path root;
        {
            std::lock_guard lock(g_mu);
            root = g_workspace_root;
        }
        if (root.empty())
            return {};
        return root / ".noni" / "lsp";
    }

    fs::path tmp_lsp_dir()
    {
        return fs::path("/tmp") /
               std::format("noni-lsp-{}", static_cast<unsigned>(::getuid()));
    }

    void set_status(const std::string &name, LspInstaller::Status st)
    {
        std::lock_guard lock(g_mu);
        g_status[name] = st;
    }

    fs::path which_on_path(const std::string &name)
    {
        if (name.find('/') != std::string::npos)
        {
            std::error_code ec;
            if (fs::is_regular_file(name, ec) && !ec)
                return name;
            return {};
        }
        const char *path = std::getenv("PATH");
        if (!path)
            return {};
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
                    return cand;
            }
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        return {};
    }
}
