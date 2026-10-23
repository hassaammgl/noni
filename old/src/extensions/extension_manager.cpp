#include <extensions/extension_manager.hpp>
#include <utils/logger.hpp>

#include <format>

void ExtensionManager::add(std::unique_ptr<Extension> ext)
{
    if (!ext)
        return;
    Entry e;
    e.ext = std::move(ext);
    e.state = ExtensionState::Loaded;
    entries_.push_back(std::move(e));
}

void ExtensionManager::activate_all(
    CommandRegistry &commands,
    KeybindingEngine &keys,
    EditorCore &core,
    const AppConfig &config)
{
    if (activated_)
        return;
    activated_ = true;

    for (auto &e : entries_)
    {
        if (!e.ext)
        {
            e.state = ExtensionState::Failed;
            e.error = "null extension";
            continue;
        }
        try
        {
            e.ctx = std::make_unique<ExtensionContext>(
                e.ext->id(), commands, keys, core, config);
            e.ext->activate(*e.ctx);
            e.state = ExtensionState::Activated;
            Logger::info(std::format("Extension activated: {} ({})", e.ext->id(), e.ext->name()));
        }
        catch (const std::exception &ex)
        {
            e.state = ExtensionState::Failed;
            e.error = ex.what();
            if (e.ctx)
                e.ctx->dispose();
            e.ctx.reset();
            Logger::error(std::format(
                "Extension activate failed: {} — {}",
                e.ext->id(),
                ex.what()));
        }
        catch (...)
        {
            e.state = ExtensionState::Failed;
            e.error = "unknown error";
            if (e.ctx)
                e.ctx->dispose();
            e.ctx.reset();
            Logger::error(std::format("Extension activate failed: {}", e.ext->id()));
        }
    }
}

void ExtensionManager::deactivate_all()
{
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
    {
        auto &e = *it;
        if (e.state != ExtensionState::Activated)
            continue;
        try
        {
            e.ext->deactivate();
        }
        catch (const std::exception &ex)
        {
            Logger::warning(std::format(
                "Extension deactivate error: {} — {}",
                e.ext ? e.ext->id() : "?",
                ex.what()));
        }
        catch (...)
        {
            Logger::warning("Extension deactivate error (unknown)");
        }
        if (e.ctx)
            e.ctx->dispose();
        e.ctx.reset();
        e.state = ExtensionState::Deactivated;
    }
    activated_ = false;
}

std::size_t ExtensionManager::activated_count() const
{
    std::size_t n = 0;
    for (const auto &e : entries_)
    {
        if (e.state == ExtensionState::Activated)
            ++n;
    }
    return n;
}

std::string ExtensionManager::status_summary() const
{
    return std::format(
        "Extensions: {} loaded, {} active",
        entries_.size(),
        activated_count());
}
