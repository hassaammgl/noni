#pragma once

#include <commands/command.hpp>

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Maps CommandId → handler. Supports optional ownership for extension cleanup.
class CommandRegistry
{
private:
    std::unordered_map<CommandId, std::function<void()>> handlers_;
    std::unordered_map<CommandId, std::string> owners_;
    std::unordered_set<CommandId> known_;

public:
    static constexpr const char *kCoreOwner = "noni.core";

    void register_command(const CommandId &id, std::function<void()> handler)
    {
        register_command(id, std::move(handler), kCoreOwner);
    }

    void register_command(const CommandId &id, std::function<void()> handler, std::string owner)
    {
        known_.insert(id);
        handlers_[id] = std::move(handler);
        owners_[id] = std::move(owner);
    }

    // Remove all commands owned by `owner`. Core owner is never cleared this way.
    void unregister_owner(const std::string &owner)
    {
        if (owner.empty() || owner == kCoreOwner)
            return;
        std::vector<CommandId> drop;
        for (const auto &[id, o] : owners_)
        {
            if (o == owner)
                drop.push_back(id);
        }
        for (const auto &id : drop)
        {
            handlers_.erase(id);
            owners_.erase(id);
            known_.erase(id);
        }
    }

    bool execute(const CommandId &id) const
    {
        auto it = handlers_.find(id);
        if (it == handlers_.end() || !it->second)
            return false;
        it->second();
        return true;
    }

    bool has(const CommandId &id) const
    {
        return handlers_.find(id) != handlers_.end();
    }

    std::string owner_of(const CommandId &id) const
    {
        auto it = owners_.find(id);
        return it == owners_.end() ? std::string{} : it->second;
    }

    const std::unordered_set<CommandId> &known() const { return known_; }

    void diagnose(
        const std::vector<CommandId> &bound_ids,
        std::vector<CommandId> *unbound_out,
        std::vector<CommandId> *unknown_out) const;
};
