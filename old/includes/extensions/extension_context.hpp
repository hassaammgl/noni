#pragma once

#include <commands/command_registry.hpp>
#include <configs/config.hpp>
#include <configs/keybindings.hpp>
#include <editor/editor_core.hpp>
#include <extensions/editor_events.hpp>

#include <functional>
#include <string>
#include <vector>

// Narrow service face for extensions. Not a dump of UI/ncurses internals.
class ExtensionContext
{
public:
    ExtensionContext(
        std::string extension_id,
        CommandRegistry &commands,
        KeybindingEngine &keys,
        EditorCore &core,
        const AppConfig &config);

    const std::string &extension_id() const { return extension_id_; }

    void register_command(const CommandId &id, std::function<void()> handler);

    // Runtime binding (owner-tagged). Prefer config.json for user bindings.
    bool add_keybinding(const std::string &key, const CommandId &command, const std::string &when = "");

    std::uint64_t on_buffer_opened(EditorEvents::BufferFn fn);
    std::uint64_t on_buffer_changed(EditorEvents::BufferChangeFn fn);
    std::uint64_t on_buffer_saved(EditorEvents::BufferFn fn);
    std::uint64_t on_buffer_closed(EditorEvents::BufferFn fn);

    EditorCore &core() { return core_; }
    const EditorCore &core() const { return core_; }
    const AppConfig &config() const { return config_; }

    // Namespaced: config.extensions[extension_id]
    MiniJson::Value extension_config() const;

    void info(const std::string &msg) const;
    void warning(const std::string &msg) const;

    // Drop commands/events/keybindings owned by this extension.
    void dispose();

private:
    std::string extension_id_;
    CommandRegistry &commands_;
    KeybindingEngine &keys_;
    EditorCore &core_;
    const AppConfig &config_;
    std::vector<std::uint64_t> event_subs_;
};
