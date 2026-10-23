#include <lsp/lsp_installer.hpp>
#include "lsp_installer_detail.hpp"
#include "lsp_installer_state.hpp"
#include <utils/async.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <format>
using namespace lsp_installer_state;

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
