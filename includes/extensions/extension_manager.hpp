#pragma once

#include <extensions/extension_context.hpp>

#include <memory>
#include <string>
#include <vector>

enum class ExtensionState
{
    Discovered,
    Loaded,
    Activated,
    Deactivated,
    Failed,
};

class Extension
{
public:
    virtual ~Extension() = default;

    virtual std::string id() const = 0;
    virtual std::string name() const = 0;
    virtual std::string version() const { return "0.1.0"; }

    virtual void activate(ExtensionContext &ctx) = 0;
    virtual void deactivate() {}
};

class ExtensionManager
{
public:
    void add(std::unique_ptr<Extension> ext);
    void activate_all(
        CommandRegistry &commands,
        KeybindingEngine &keys,
        EditorCore &core,
        const AppConfig &config);
    void deactivate_all();

    std::size_t count() const { return entries_.size(); }
    std::size_t activated_count() const;

    std::string status_summary() const;

private:
    struct Entry
    {
        std::unique_ptr<Extension> ext;
        std::unique_ptr<ExtensionContext> ctx;
        ExtensionState state = ExtensionState::Discovered;
        std::string error;
    };

    std::vector<Entry> entries_;
    bool activated_ = false;
};
