#include <extensions/extension_context.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>

#include <format>

ExtensionContext::ExtensionContext(
    std::string extension_id,
    CommandRegistry &commands,
    KeybindingEngine &keys,
    EditorCore &core,
    const AppConfig &config)
    : extension_id_(std::move(extension_id)),
      commands_(commands),
      keys_(keys),
      core_(core),
      config_(config)
{
}

void ExtensionContext::register_command(const CommandId &id, std::function<void()> handler)
{
    commands_.register_command(id, std::move(handler), extension_id_);
}

bool ExtensionContext::add_keybinding(
    const std::string &key,
    const CommandId &command,
    const std::string &when)
{
    return keys_.add_binding(key, command, when, extension_id_);
}

std::uint64_t ExtensionContext::on_buffer_opened(EditorEvents::BufferFn fn)
{
    const auto id = core_.events().on_buffer_opened(std::move(fn), extension_id_);
    event_subs_.push_back(id);
    return id;
}

std::uint64_t ExtensionContext::on_buffer_changed(EditorEvents::BufferChangeFn fn)
{
    const auto id = core_.events().on_buffer_changed(std::move(fn), extension_id_);
    event_subs_.push_back(id);
    return id;
}

std::uint64_t ExtensionContext::on_buffer_saved(EditorEvents::BufferFn fn)
{
    const auto id = core_.events().on_buffer_saved(std::move(fn), extension_id_);
    event_subs_.push_back(id);
    return id;
}

std::uint64_t ExtensionContext::on_buffer_closed(EditorEvents::BufferFn fn)
{
    const auto id = core_.events().on_buffer_closed(std::move(fn), extension_id_);
    event_subs_.push_back(id);
    return id;
}

MiniJson::Value ExtensionContext::extension_config() const
{
    auto it = config_.extensions.find(extension_id_);
    if (it == config_.extensions.end())
        return MiniJson::Value{MiniJson::Object{}};
    return it->second;
}

void ExtensionContext::info(const std::string &msg) const
{
    Messages::info(std::format("[{}] {}", extension_id_, msg));
    Logger::info(std::format("ext {}: {}", extension_id_, msg));
}

void ExtensionContext::warning(const std::string &msg) const
{
    Messages::warning(std::format("[{}] {}", extension_id_, msg));
    Logger::warning(std::format("ext {}: {}", extension_id_, msg));
}

void ExtensionContext::dispose()
{
    commands_.unregister_owner(extension_id_);
    keys_.remove_bindings_owned_by(extension_id_);
    core_.events().unsubscribe_owner(extension_id_);
    event_subs_.clear();
}
