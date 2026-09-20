#include <commands/command_registry.hpp>
#include <utils/logger.hpp>

#include <format>

void CommandRegistry::diagnose(
    const std::vector<CommandId> &bound_ids,
    std::vector<CommandId> *unbound_out,
    std::vector<CommandId> *unknown_out) const
{
    std::unordered_set<CommandId> bound(bound_ids.begin(), bound_ids.end());

    if (unbound_out)
    {
        for (const auto &id : known_)
        {
            if (!bound.count(id))
            {
                unbound_out->push_back(id);
            }
        }
    }

    if (unknown_out)
    {
        for (const auto &id : bound_ids)
        {
            if (!handlers_.count(id))
            {
                unknown_out->push_back(id);
            }
        }
    }
}
