#include <lsp/lsp_installer.hpp>
#include "lsp_installer_detail.hpp"

#include <utils/async.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <mutex>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace
{
    std::mutex g_mu;
    bool g_auto_install = true;
    std::atomic<std::uint64_t> g_generation{1};
    std::unordered_map<std::string, LspInstaller::Status> g_status;
    std::unordered_set<std::string> g_queued;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> g_failed_at;
    constexpr auto kFailedRetry = std::chrono::seconds(120);
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

void LspInstaller::set_auto_install(bool enabled)
{
    std::lock_guard lock(g_mu);
    g_auto_install = enabled;
}

bool LspInstaller::auto_install()
{
    std::lock_guard lock(g_mu);
    return g_auto_install;
}

void LspInstaller::set_workspace_root(const fs::path &root)
{
    std::lock_guard lock(g_mu);
    if (g_workspace_root == root)
        return;
    g_workspace_root = root;
    g_user_dir_resolved = false;
    g_user_dir_cache.clear();
}

fs::path LspInstaller::user_dir()
{
    {
        std::lock_guard lock(g_mu);
        if (g_user_dir_resolved)
            return g_user_dir_cache;
    }

    const fs::path preferred = xdg_lsp_dir();
    if (dir_writable(preferred))
    {
        std::lock_guard lock(g_mu);
        g_user_dir_cache = preferred;
        g_user_dir_resolved = true;
        return g_user_dir_cache;
    }

    const fs::path ws = workspace_lsp_dir();
    if (!ws.empty() && dir_writable(ws))
    {
        Logger::warning(std::format(
            "lsp-install: {} not writable — using {}",
            preferred.string(),
            ws.string()));
        std::lock_guard lock(g_mu);
        g_user_dir_cache = ws;
        g_user_dir_resolved = true;
        return g_user_dir_cache;
    }

    const fs::path tmp = tmp_lsp_dir();
    std::error_code ec;
    fs::create_directories(tmp, ec);
    Logger::warning(std::format(
        "lsp-install: {} not writable — using {}",
        preferred.string(),
        tmp.string()));
    std::lock_guard lock(g_mu);
    g_user_dir_cache = tmp;
    g_user_dir_resolved = true;
    return g_user_dir_cache;
}

fs::path LspInstaller::bin_dir()
{
    return user_dir() / "bin";
}

fs::path LspInstaller::resolve(const std::string &command_name)
{
    if (command_name.empty())
        return {};

    if (const fs::path on_path = which_on_path(command_name); !on_path.empty())
        return on_path;

    const fs::path local = bin_dir() / command_name;
    std::error_code ec;
    if (fs::is_regular_file(local, ec) && !ec)
        return local;

    const fs::path xdg = xdg_lsp_dir() / "bin" / command_name;
    if (fs::is_regular_file(xdg, ec) && !ec)
        return xdg;

    if (const fs::path ws = workspace_lsp_dir(); !ws.empty())
    {
        const fs::path cand = ws / "bin" / command_name;
        if (fs::is_regular_file(cand, ec) && !ec)
            return cand;
    }
    return {};
}

std::uint64_t LspInstaller::generation()
{
    return g_generation.load(std::memory_order_relaxed);
}

LspInstaller::Status LspInstaller::status(const std::string &command_name)
{
    {
        std::lock_guard lock(g_mu);
        if (const auto it = g_status.find(command_name); it != g_status.end())
            return it->second;
    }
    if (!resolve(command_name).empty())
        return Status::Ready;
    return Status::Missing;
}

void LspInstaller::request(const std::string &command_name, bool force)
{
    if (command_name.empty() || command_name.find('/') != std::string::npos)
        return;

    if (!resolve(command_name).empty())
    {
        set_status(command_name, Status::Ready);
        return;
    }

    const auto *spec = lsp_install_detail::spec_for(command_name);
    if (!spec)
    {
        Logger::warning(std::format(
            "lsp-install: no auto-install recipe for `{}`",
            command_name));
        Messages::warning(std::format(
            "LSP `{}` missing (no auto-install recipe)",
            command_name));
        set_status(command_name, Status::Failed);
        return;
    }

    {
        std::lock_guard lock(g_mu);
        if (!force && !g_auto_install)
            return;
        if (g_queued.count(command_name) || g_status[command_name] == Status::Installing)
            return;
        if (!force && g_status[command_name] == Status::Failed)
        {
            const auto it = g_failed_at.find(command_name);
            if (it != g_failed_at.end() &&
                std::chrono::steady_clock::now() - it->second < kFailedRetry)
                return;
        }
        g_queued.insert(command_name);
        g_status[command_name] = Status::Installing;
        g_failed_at.erase(command_name);
    }

    const std::string name = command_name;
    const lsp_install_detail::Spec spec_copy = *spec;
    Background::instance().post([name, spec_copy]() {
        const bool ok = lsp_install_detail::install_one(spec_copy);
        {
            std::lock_guard lock(g_mu);
            g_queued.erase(name);
            g_status[name] = ok ? Status::Ready : Status::Failed;
            if (!ok)
                g_failed_at[name] = std::chrono::steady_clock::now();
            else
                g_failed_at.erase(name);
        }
        if (ok)
            g_generation.fetch_add(1, std::memory_order_relaxed);
    });
}

void LspInstaller::request_all(const std::vector<std::string> &command_names)
{
    for (const auto &n : command_names)
        request(n);
}

const char *LspInstaller::status_label(Status st)
{
    switch (st)
    {
    case Status::Missing:
        return "missing";
    case Status::Installing:
        return "installing";
    case Status::Ready:
        return "ready";
    case Status::Failed:
        return "failed";
    }
    return "?";
}

std::vector<LspInstaller::CatalogEntry> LspInstaller::catalog()
{
    std::size_t n = 0;
    const auto *specs = lsp_install_detail::catalog_specs(n);
    std::vector<CatalogEntry> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        CatalogEntry e;
        e.binary = specs[i].binary;
        e.package = specs[i].package;
        switch (specs[i].method)
        {
        case lsp_install_detail::Method::PipVenv:
            e.via = "pip";
            break;
        case lsp_install_detail::Method::NpmPrefix:
            e.via = "npm/bun";
            break;
        case lsp_install_detail::Method::CurlGithubZip:
            e.via = "curl";
            break;
        case lsp_install_detail::Method::GoInstall:
            e.via = "go";
            break;
        }
        out.push_back(std::move(e));
    }
    return out;
}
