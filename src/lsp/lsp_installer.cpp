#include <lsp/lsp_installer.hpp>
#include "lsp_installer_detail.hpp"
#include "lsp_installer_state.hpp"
#include <utils/async.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <format>
using namespace lsp_installer_state;

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

