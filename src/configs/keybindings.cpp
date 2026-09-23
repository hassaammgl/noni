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
#include "keybindings_ext.hpp"

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

void KeybindingEngine::register_extended_keys()
{
    keybindings_ext::register_extended_keys();
}

const char *KeybindingEngine::extended_seq(int raw_key)
{
    return keybindings_ext::seq_for(raw_key);
}

std::string KeybindingEngine::pending_label() const
{
    if (pending.empty())
        return {};
    std::string s;
    for (const auto &t : pending)
    {
        if (!s.empty())
            s += ' ';
        if (t.ctrl)
            s += "Ctrl+";
        if (t.alt)
            s += "Alt+";
        if (t.shift && (t.code < 'A' || t.code > 'Z'))
            s += "Shift+";
        if (t.code == ' ')
            s += "Space";
        else if (t.code == 27)
            s += "Esc";
        else if (t.code == '\t')
            s += "Tab";
        else if (t.code == '\n')
            s += "Enter";
        else if (t.code == 127)
            s += "Bs";
        else if (t.code == KEY_UP)
            s += "Up";
        else if (t.code == KEY_DOWN)
            s += "Down";
        else if (t.code == KEY_LEFT)
            s += "Left";
        else if (t.code == KEY_RIGHT)
            s += "Right";
        else if (t.code >= KEY_F(1) && t.code <= KEY_F(12))
            s += std::format("F{}", t.code - KEY_F(1) + 1);
        else if (t.code >= 32 && t.code < 127)
            s += static_cast<char>(t.code);
        else if (const char *nm = keyname(t.code))
            s += nm;
        else
            s += '?';
    }
    s += "…";
    return s;
}
