#include <syntax/grammar_installer.hpp>
#include "grammar_state.hpp"
#include <utils/async.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <format>
using namespace grammar_install_state;


std::uint64_t GrammarInstaller::generation()
{
    return g_generation.load(std::memory_order_relaxed);
}

GrammarInstaller::Status GrammarInstaller::status(const std::string &so_name)
{
    {
        std::lock_guard lock(g_mu);
        if (const auto it = g_status.find(so_name); it != g_status.end())
            return it->second;
    }
    if (!find_grammar(so_name).empty())
        return Status::Ready;
    return Status::Missing;
}

void GrammarInstaller::request(Language lang)
{
    const char *so = so_for_language(lang);
    if (!so)
        return;

    const InstallSpec *spec = install_spec(so);
    if (!spec)
        return;

    if (!find_grammar(so).empty())
    {
        set_status(so, Status::Ready);
        return;
    }

    {
        std::lock_guard lock(g_mu);
        if (!g_auto_install)
            return;
        if (g_queued.count(so) || g_status[so] == Status::Installing)
            return;
        if (g_status[so] == Status::Failed)
        {
            const auto it = g_failed_at.find(so);
            if (it != g_failed_at.end() &&
                std::chrono::steady_clock::now() - it->second < kFailedRetry)
                return;
        }
        g_queued.insert(so);
        g_status[so] = Status::Installing;
        g_failed_at.erase(so);
    }

    const fs::path dest = GrammarInstaller::user_dir();
    Messages::info(std::format(
        "Installing {} → {} …",
        so,
        dest.string()));
    Logger::info(std::format(
        "grammar-install: queued {} → {}",
        so,
        dest.string()));

    const std::string so_name = so;
    InstallSpec spec_copy = *spec;

    Background::instance().post([so_name, spec_copy]() {
        const bool ok = install_one(spec_copy);
        const fs::path out = GrammarInstaller::so_path(so_name);
        {
            std::lock_guard lock(g_mu);
            g_queued.erase(so_name);
            g_status[so_name] = ok ? Status::Ready : Status::Failed;
            if (!ok)
                g_failed_at[so_name] = std::chrono::steady_clock::now();
            else
                g_failed_at.erase(so_name);
        }
        if (ok)
        {
            g_generation.fetch_add(1, std::memory_order_relaxed);
            Messages::info(std::format("Grammar ready: {}", out.string()));
        }
        else
        {
            Messages::warning(std::format(
                "Grammar failed: {} (retry 60s; log: logs/grammar-install.log)",
                so_name));
        }
    });
}
