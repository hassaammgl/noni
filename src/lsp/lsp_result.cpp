#include "lsp_internal.hpp"

std::optional<CompletionList> LspService::take_completion_result()
{
    std::lock_guard lock(mu_);
    if (!ready_completion_)
        return std::nullopt;
    auto out = std::move(*ready_completion_);
    ready_completion_.reset();
    return out;
}

void LspService::cancel_completion()
{
    cancel_pending();
}

void LspService::clear_ready()
{
    ready_completion_.reset();
    ready_locations_.reset();
    ready_symbols_.reset();
    ready_rename_.reset();
    ready_actions_.reset();
    ready_server_apply_.reset();
}

void LspService::cancel_pending()
{
    std::lock_guard lock(mu_);
    pending_.clear();
    clear_ready();
}

std::optional<LspLocationList> LspService::take_location_result()
{
    std::lock_guard lock(mu_);
    if (!ready_locations_)
        return std::nullopt;
    auto out = std::move(*ready_locations_);
    ready_locations_.reset();
    return out;
}

std::optional<LspSymbolList> LspService::take_symbol_result()
{
    std::lock_guard lock(mu_);
    if (!ready_symbols_)
        return std::nullopt;
    auto out = std::move(*ready_symbols_);
    ready_symbols_.reset();
    return out;
}

std::optional<LspRenameResult> LspService::take_rename_result()
{
    std::lock_guard lock(mu_);
    if (!ready_rename_)
        return std::nullopt;
    auto out = std::move(*ready_rename_);
    ready_rename_.reset();
    return out;
}

std::optional<LspCodeActionList> LspService::take_code_action_result()
{
    std::lock_guard lock(mu_);
    if (!ready_actions_)
        return std::nullopt;
    auto out = std::move(*ready_actions_);
    ready_actions_.reset();
    return out;
}

std::optional<LspWorkspaceEdit> LspService::take_server_apply_edit()
{
    std::lock_guard lock(mu_);
    if (!ready_server_apply_)
        return std::nullopt;
    auto out = std::move(*ready_server_apply_);
    ready_server_apply_.reset();
    return out;
}
