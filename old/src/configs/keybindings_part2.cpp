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

    if (pending.empty())
    {
        result.status = ResolveStatus::Unmatched;
        return result;
    }

    FirstKey k{
        .code = pending.front().code,
        .ctrl = pending.front().ctrl,
        .alt = pending.front().alt,
        .shift = pending.front().shift};
    const auto it = first_index_.find(k);
    if (it == first_index_.end())
    {
        pending.clear();
        result.status = ResolveStatus::Unmatched;
        return result;
    }

    for (const std::size_t bi : it->second)
    {
        const auto &b = bindings[bi];
        if (!when_matches(b.when, when_context))
            continue;
        if (b.chord.size() < pending.size())
            continue;

        if (b.leader && !context_allows_leader(input_ctx))
            continue;

        const bool editor_modal =
            input_ctx == InputContext::EditorNormal ||
            input_ctx == InputContext::EditorVisual;
        if (editor_modal && !b.chord.empty() && !engine_owns_token(b.chord.front()))
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
    }

    if (!any_prefix)
    {
        pending.clear();
        result.status = ResolveStatus::Unmatched;
        return result;
    }

    result.status = ResolveStatus::Prefix;
    return result;
}

ResolveResult KeybindingEngine::resolve_token(KeyToken tok, const std::string &when_context, InputContext input_ctx)
{
    const bool editor_modal =
        input_ctx == InputContext::EditorNormal ||
        input_ctx == InputContext::EditorVisual;

    // Vim prefixes (g u m ' " [ ] d/c/y …) stay in editor.cpp — do not start engine chords.
    if (editor_modal && !has_pending() && !engine_owns_token(tok))
    {
        ResolveResult r;
        r.status = ResolveStatus::Unmatched;
        return r;
    }

    // Literal contexts: only allow Esc / explicit control bindings (single-key),
    // never start a Space leader chord.
    if (context_is_literal(input_ctx) && !has_pending())
    {
        // Allow Esc and F-keys to match single-key bindings.
        const bool controlish =
            tok.code == 27 || tok.ctrl || tok.alt ||
            (tok.code >= KEY_F(1) && tok.code <= KEY_F(12));
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

ResolveResult KeybindingEngine::resolve(int raw_key, const std::string &when_context, InputContext input_ctx)
{
    return resolve_token(from_raw(raw_key), when_context, input_ctx);
}
