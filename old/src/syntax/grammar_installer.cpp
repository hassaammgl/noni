#include <syntax/grammar_installer.hpp>
#include "grammar_state.hpp"
#include <utils/async.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <format>
using namespace grammar_install_state;

void GrammarInstaller::set_auto_install(bool enabled)
{
    std::lock_guard lock(g_mu);
    g_auto_install = enabled;
}

bool GrammarInstaller::auto_install()
{
    std::lock_guard lock(g_mu);
    return g_auto_install;
}

void GrammarInstaller::set_workspace_root(const fs::path &root)
{
    std::lock_guard lock(g_mu);
    if (g_workspace_root == root)
        return;
    g_workspace_root = root;
    // Re-resolve install dir so we can prefer <ws>/.noni/tree-sitter.
    g_user_dir_resolved = false;
    g_user_dir_cache.clear();
}

fs::path GrammarInstaller::user_dir()
{
    {
        std::lock_guard lock(g_mu);
        if (g_user_dir_resolved)
            return g_user_dir_cache;
    }

    const fs::path preferred = xdg_grammar_dir();
    if (dir_writable(preferred))
    {
        std::lock_guard lock(g_mu);
        g_user_dir_cache = preferred;
        g_user_dir_resolved = true;
        return g_user_dir_cache;
    }

    const fs::path ws = workspace_grammar_dir();
    if (!ws.empty() && dir_writable(ws))
    {
        Logger::warning(std::format(
            "grammar-install: {} not writable — using {} "
            "(fix with: sudo chown -R \"$USER\" ~/.local/share/noni)",
            preferred.string(),
            ws.string()));
        std::lock_guard lock(g_mu);
        g_user_dir_cache = ws;
        g_user_dir_resolved = true;
        return g_user_dir_cache;
    }

    const fs::path tmp = tmp_grammar_dir();
    std::error_code ec;
    fs::create_directories(tmp, ec);
    Logger::warning(std::format(
        "grammar-install: {} not writable — using {} "
        "(fix with: sudo chown -R \"$USER\" ~/.local/share/noni)",
        preferred.string(),
        tmp.string()));
    std::lock_guard lock(g_mu);
    g_user_dir_cache = tmp;
    g_user_dir_resolved = true;
    return g_user_dir_cache;
}

fs::path GrammarInstaller::so_path(const std::string &so_name)
{
    return user_dir() / so_name;
}

fs::path GrammarInstaller::find_grammar(const std::string &so_name)
{
    const fs::path system = fs::path("/usr/lib/tree_sitter") / so_name;
    if (fs::exists(system))
        return system;

    // Preferred XDG path (may be root-owned and read-only — still load .so from it).
    {
        const fs::path preferred = xdg_grammar_dir() / so_name;
        if (fs::exists(preferred))
            return preferred;
    }

    // Project-local cache (used when XDG is broken / unwritable).
    if (const fs::path ws = workspace_grammar_dir(); !ws.empty())
    {
        const fs::path cand = ws / so_name;
        if (fs::exists(cand))
            return cand;
    }

    const fs::path user = so_path(so_name); // writable dir (xdg / .noni / /tmp)
    if (fs::exists(user))
        return user;

    const fs::path tmp = tmp_grammar_dir() / so_name;
    if (fs::exists(tmp))
        return tmp;

    return {};
}
