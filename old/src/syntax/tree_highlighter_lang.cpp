#include <syntax/tree_highlighter.hpp>
#include <syntax/grammar_installer.hpp>
#include <utils/logger.hpp>
#include <tree_sitter/api.h>
#include <dlfcn.h>
#include <algorithm>
#include <cstring>
#include <format>
#include "ts_hl_detail.hpp"
using namespace ts_hl_detail;

bool TreeHighlighter::ensure_language(Language lang)
{
    if (parser && query && ts_language && loaded_lang == lang)
        return true;

    const LangSpec *spec = spec_for(lang);
    if (!spec)
        return false;

    auto try_load = [&](const char *so, const char *sym, const char *qsrc, Language mark) -> bool {
        clear_ts();
        const fs::path path = GrammarInstaller::find_grammar(so);
        if (path.empty())
            return false;

        dl_handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!dl_handle)
            return false;

        auto *fn = reinterpret_cast<LangFn>(dlsym(dl_handle, sym));
        if (!fn)
        {
            clear_ts();
            return false;
        }

        const TSLanguage *language = fn();
        if (!language)
        {
            clear_ts();
            return false;
        }

        parser = ts_parser_new();
        if (!ts_parser_set_language(static_cast<TSParser *>(parser), language))
        {
            Logger::warning(std::format(
                "tree-sitter ABI mismatch for {}", so));
            clear_ts();
            return false;
        }

        uint32_t err_off = 0;
        TSQueryError err = TSQueryErrorNone;
        query = ts_query_new(language, qsrc, static_cast<uint32_t>(std::strlen(qsrc)), &err_off, &err);
        if (!query)
        {
            Logger::warning(std::format(
                "tree-sitter query failed for {} (err={} at {})",
                so,
                static_cast<int>(err),
                err_off));
            clear_ts();
            return false;
        }

        ts_language = language;
        loaded_lang = mark;
        return true;
    };

    if (try_load(spec->so_name, spec->symbol, spec->query, lang))
        return true;

    if (lang == Language::Cpp)
    {
        const LangSpec *c = spec_for(Language::C);
        if (c && try_load(c->so_name, c->symbol, c->query, lang))
            return true;
    }

    GrammarInstaller::request(lang);
    if (lang == Language::Cpp)
        GrammarInstaller::request(Language::C);

    loaded_lang = Language::Plain;
    return false;
}

void TreeHighlighter::apply_query()
{
    if (!tree || !query)
        return;

    for (auto &row : line_tokens)
        row.clear();

    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(
        cursor,
        static_cast<TSQuery *>(query),
        ts_tree_root_node(static_cast<TSTree *>(tree)));

    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match))
    {
        for (uint16_t i = 0; i < match.capture_count; ++i)
        {
            const TSQueryCapture &cap = match.captures[i];
            uint32_t name_len = 0;
            const char *name = ts_query_capture_name_for_id(
                static_cast<TSQuery *>(query),
                cap.index,
                &name_len);
            const TokenKind kind = capture_to_kind(
                std::string_view(name, name_len));

            const TSNode node = cap.node;
            const TSPoint start = ts_node_start_point(node);
            const TSPoint end = ts_node_end_point(node);
            add_span(
                line_tokens,
                start.row,
                start.column,
                end.row,
                end.column,
                kind);
        }
    }

    ts_query_cursor_delete(cursor);

    for (auto &toks : line_tokens)
    {
        std::sort(toks.begin(), toks.end(), [](const SyntaxToken &a, const SyntaxToken &b) {
            if (a.start != b.start)
                return a.start < b.start;
            return a.length > b.length;
        });
    }
}

void TreeHighlighter::clip_tokens(const std::vector<std::string> &lines)
{
    for (std::size_t i = 0; i < line_tokens.size() && i < lines.size(); ++i)
    {
        const int n = static_cast<int>(lines[i].size());
        for (auto &tok : line_tokens[i])
        {
            if (tok.start >= n)
            {
                tok.length = 0;
                continue;
            }
            tok.length = std::min(tok.length, n - tok.start);
        }
    }
}

