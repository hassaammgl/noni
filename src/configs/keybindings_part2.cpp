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
