#pragma once

#include <commands/command.hpp>

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Maps CommandId → handler. Does not own UI; handlers close over application.
class CommandRegistry
{
private:
    std::unordered_map<CommandId, std::function<void()>> handlers_;
    std::unordered_set<CommandId> known_;

public:
    void register_command(const CommandId &id, std::function<void()> handler)
    {
        known_.insert(id);
        handlers_[id] = std::move(handler);
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

    const std::unordered_set<CommandId> &known() const { return known_; }

    // Diagnostics: unbound registered commands / unknown binding targets.
    void diagnose(
        const std::vector<CommandId> &bound_ids,
        std::vector<CommandId> *unbound_out,
        std::vector<CommandId> *unknown_out) const;
};
