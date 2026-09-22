#include <configs/keybindings.hpp>
#include <commands/command_registry.hpp>
#include <utils/logger.hpp>
#include <utils/str.hpp>

#include <format>

#include <ncurses.h>
#include <algorithm>
#include <cctype>
#include <sstream>

#include "keybindings_detail.hpp"

using namespace keybindings_detail;

void KeybindingEngine::load(const AppConfig &config)
{
    bindings.clear();
    pending.clear();

    for (const auto &kb : config.keybindings)
    {
        ResolvedBinding rb;
        rb.chord = parse_key(kb.key);
        rb.command = kb.command;
        rb.when = kb.when;
        rb.owner = CommandRegistry::kCoreOwner;
        if (!rb.chord.empty() && !rb.command.empty())
        {
            rb.leader = is_leader_token(rb.chord.front());
            bindings.push_back(std::move(rb));
        }
        else
        {
            Logger::warning(std::format(
                "keybinding skipped (unparsed key or empty command): '{}' -> '{}'",
                kb.key,
                kb.command));
        }
    }
}

bool KeybindingEngine::add_binding(
    const std::string &key,
    const CommandId &command,
    const std::string &when,
    const std::string &owner)
{
    ResolvedBinding rb;
    rb.chord = parse_key(key);
    rb.command = command;
    rb.when = when;
    rb.owner = owner;
    if (rb.chord.empty() || rb.command.empty())
        return false;
    rb.leader = is_leader_token(rb.chord.front());
    bindings.push_back(std::move(rb));
    return true;
}

void KeybindingEngine::remove_bindings_owned_by(const std::string &owner)
{
    if (owner.empty())
        return;
    bindings.erase(
        std::remove_if(
            bindings.begin(),
            bindings.end(),
            [&owner](const ResolvedBinding &b) { return b.owner == owner; }),
        bindings.end());
}

void KeybindingEngine::clear_chord()
{
    pending.clear();
}

bool KeybindingEngine::is_leader_token(const KeyToken &t) const
{
    return !t.ctrl && !t.alt && t.code == ' ';
}

std::vector<CommandId> KeybindingEngine::bound_commands() const
{
    std::vector<CommandId> out;
    out.reserve(bindings.size());
    for (const auto &b : bindings)
        out.push_back(b.command);
    return out;
}
