#include <configs/keybindings.hpp>
#include <commands/command_registry.hpp>
#include <utils/logger.hpp>
#include <utils/str.hpp>

#include <format>

#include <ncurses.h>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace
{
    std::string lower(std::string s)
    {
        for (char &c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    KeyToken parse_single(std::string part)
    {
        part = StrUtils::trim(part);
        KeyToken t;

        while (true)
        {
            const auto plus = part.find('+');
            if (plus == std::string::npos)
                break;

            std::string mod = lower(part.substr(0, plus));
            part = part.substr(plus + 1);
            if (mod == "ctrl" || mod == "control" || mod == "c")
                t.ctrl = true;
            else if (mod == "alt" || mod == "option" || mod == "a")
                t.alt = true;
            else if (mod == "shift" || mod == "s")
                t.shift = true;
        }

        part = StrUtils::trim(part);
        const std::string name = lower(part);

        if (name == "escape" || name == "esc")
            t.code = 27;
        else if (name == "tab")
            t.code = '\t';
        else if (name == "enter" || name == "return")
            t.code = '\n';
        else if (name == "space")
            t.code = ' ';
        else if (name == "backspace" || name == "bs")
            t.code = 127;
        else if (name == "f2")
            t.code = KEY_F(2);
        else if (name == "f3")
            t.code = KEY_F(3);
        else if (name == "f4")
            t.code = KEY_F(4);
        else if (name == "f5")
            t.code = KEY_F(5);
        else if (name == "f6")
            t.code = KEY_F(6);
        else if (name == "f7")
            t.code = KEY_F(7);
        else if (name == "f8")
            t.code = KEY_F(8);
        else if (name == "f9")
            t.code = KEY_F(9);
        else if (name == "f10")
            t.code = KEY_F(10);
        else if (name == "f11")
            t.code = KEY_F(11);
        else if (name == "f12")
            t.code = KEY_F(12);
        else if (name == "`" || name == "backtick" || name == "grave")
            t.code = '`';
        else if (name == "\\" || name == "backslash")
            t.code = '\\';
        else if (name == "[" || name == "leftbracket")
            t.code = '[';
        else if (name == "]" || name == "rightbracket")
            t.code = ']';
        else if (name == "-" || name == "minus")
            t.code = '-';
        else if (name == "=" || name == "equals" || name == "plus")
            t.code = '=';
        else if (name == "_" || name == "underscore")
            t.code = '_';
        else if (name.size() == 1)
        {
            char c = name[0];
            if (t.shift && c >= 'a' && c <= 'z')
                c = static_cast<char>(c - 'a' + 'A');
            t.code = static_cast<unsigned char>(c);
            if (c >= 'A' && c <= 'Z')
                t.shift = true;
        }
        else if (part.size() == 1)
        {
            t.code = static_cast<unsigned char>(part[0]);
            if (part[0] >= 'A' && part[0] <= 'Z')
                t.shift = true;
        }

        return t;
    }
}

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

KeyToken KeybindingEngine::from_raw(int raw_key)
{
    KeyToken t;

    if (raw_key == 27)
    {
        t.code = 27;
        return t;
    }
    if (raw_key == '\t')
    {
        t.code = '\t';
        return t;
    }
    if (raw_key == '\n' || raw_key == KEY_ENTER)
    {
        t.code = '\n';
        return t;
    }
    if (raw_key == KEY_BACKSPACE || raw_key == 127 || raw_key == 8)
    {
        t.code = 127;
        return t;
    }

    if (raw_key == KEY_F(2))
    {
        t.code = KEY_F(2);
        return t;
    }
    if (raw_key == KEY_F(3))
    {
        t.code = KEY_F(3);
        return t;
    }
    if (raw_key == KEY_F(4))
    {
        t.code = KEY_F(4);
        return t;
    }
    if (raw_key == KEY_F(5))
    {
        t.code = KEY_F(5);
        return t;
    }
    if (raw_key == KEY_F(6))
    {
        t.code = KEY_F(6);
        return t;
    }
    if (raw_key == KEY_F(7))
    {
        t.code = KEY_F(7);
        return t;
    }
    if (raw_key == KEY_F(8))
    {
        t.code = KEY_F(8);
        return t;
    }
    if (raw_key == KEY_F(9))
    {
        t.code = KEY_F(9);
        return t;
    }
    if (raw_key == KEY_F(10))
    {
        t.code = KEY_F(10);
        return t;
    }
    if (raw_key == KEY_F(11))
    {
        t.code = KEY_F(11);
        return t;
    }
    if (raw_key == KEY_F(12))
    {
        t.code = KEY_F(12);
        return t;
    }

    if (raw_key >= 1 && raw_key <= 26)
    {
        t.ctrl = true;
        t.code = 'a' + (raw_key - 1);
        return t;
    }

    // Ctrl+\ (ASCII FS) — VS Code split binding.
    if (raw_key == 28)
    {
        t.ctrl = true;
        t.code = '\\';
        return t;
    }

    // Many terminals send NUL for Ctrl+Space (and Ctrl+@).
    if (raw_key == 0)
    {
        t.ctrl = true;
        t.code = ' ';
        return t;
    }

    if (raw_key >= 'A' && raw_key <= 'Z')
    {
        t.shift = true;
        t.code = raw_key;
        return t;
    }

    t.code = raw_key;
    return t;
}

std::vector<KeyToken> KeybindingEngine::parse_key(const std::string &spec)
{
    std::vector<KeyToken> out;
    std::istringstream iss(spec);
    std::string part;
    while (iss >> part)
        out.push_back(parse_single(part));
    return out;
}

bool KeybindingEngine::when_matches(const std::string &when, const std::string &context)
{
    if (when.empty())
        return true;

    auto has = [&](std::string_view flag)
    {
        const std::string padded = " " + context + " ";
        return padded.find(" " + std::string(flag) + " ") != std::string::npos;
    };

    auto eval_and = [&](const std::string &expr)
    {
        std::size_t start = 0;
        while (start < expr.size())
        {
            auto pos = expr.find("&&", start);
            std::string part = StrUtils::trim(expr.substr(start, pos == std::string::npos ? std::string::npos : pos - start));
            if (!part.empty() && !has(part))
                return false;
            if (pos == std::string::npos)
                break;
            start = pos + 2;
        }
        return true;
    };

    std::size_t start = 0;
    while (start < when.size())
    {
        auto pos = when.find("||", start);
        std::string part = StrUtils::trim(when.substr(start, pos == std::string::npos ? std::string::npos : pos - start));
        if (!part.empty() && eval_and(part))
            return true;
        if (pos == std::string::npos)
            break;
        start = pos + 2;
    }
    return false;
}

ResolveResult KeybindingEngine::resolve_pending(const std::string &when_context, InputContext input_ctx)
{
    ResolveResult result;
    bool any_prefix = false;
    bool any_leader_prefix = false;

    for (const auto &b : bindings)
    {
        if (!when_matches(b.when, when_context))
            continue;
        if (b.chord.size() < pending.size())
            continue;

        // Leader chords only when context allows Space leader.
        if (b.leader && !context_allows_leader(input_ctx))
            continue;

        bool prefix = true;
        for (std::size_t i = 0; i < pending.size(); ++i)
        {
            if (!(b.chord[i] == pending[i]))
            {
                prefix = false;
                break;
            }
        }
        if (!prefix)
            continue;

        if (b.chord.size() == pending.size())
        {
            pending.clear();
            result.status = ResolveStatus::Matched;
            result.command_id = b.command;
            return result;
        }

        any_prefix = true;
        if (b.leader)
            any_leader_prefix = true;
    }

    if (!any_prefix)
    {
        if (pending.size() > 1)
        {
            // Chord failed — drop the whole attempt so the last key can fall
            // through to the editor (JSON is source of truth for prefixes).
            pending.clear();
            result.status = ResolveStatus::Unmatched;
            return result;
        }
        pending.clear();
        result.status = ResolveStatus::Unmatched;
        return result;
    }

    result.status = ResolveStatus::Prefix;
    return result;
}

ResolveResult KeybindingEngine::resolve(int raw_key, const std::string &when_context, InputContext input_ctx)
{
    const KeyToken tok = from_raw(raw_key);

    // Literal contexts: only allow Esc / explicit control bindings (single-key),
    // never start a Space leader chord.
    if (context_is_literal(input_ctx) && !has_pending())
    {
        // Allow Esc and F-keys to match single-key bindings.
        const bool controlish =
            tok.code == 27 || tok.code == KEY_F(4) || tok.ctrl;
        if (!controlish && tok.code == ' ')
        {
            ResolveResult r;
            r.status = ResolveStatus::Unmatched;
            return r;
        }
        if (!controlish && !context_allows_leader(input_ctx))
        {
            // Still allow non-leader single-key bindings (e.g. escape).
            pending.push_back(tok);
            ResolveResult r = resolve_pending(when_context, input_ctx);
            if (r.status == ResolveStatus::Prefix)
            {
                pending.clear();
                r.status = ResolveStatus::Unmatched;
            }
            return r;
        }
    }

    pending.push_back(tok);
    return resolve_pending(when_context, input_ctx);
}
