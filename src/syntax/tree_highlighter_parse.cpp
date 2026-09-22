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

bool TreeHighlighter::parse_with_ts(
    const std::string &source,
    Language lang,
    bool incremental,
    std::size_t line_count)
{
    if (!ensure_language(lang))
        return false;

    TSTree *old = static_cast<TSTree *>(tree);
    if (!incremental || !old)
    {
        if (old)
        {
            ts_tree_delete(old);
            tree = nullptr;
            old = nullptr;
        }
        tree = ts_parser_parse_string(
            static_cast<TSParser *>(parser),
            nullptr,
            source.c_str(),
            static_cast<uint32_t>(source.size()));
    }
    else
    {
        tree = ts_parser_parse_string(
            static_cast<TSParser *>(parser),
            old,
            source.c_str(),
            static_cast<uint32_t>(source.size()));
        if (tree != old)
            ts_tree_delete(old);
    }

    if (!tree)
    {
        tree = nullptr;
        return false;
    }

    line_tokens.assign(line_count, {});
    apply_query();
    ts_active = true;
    cached_source = source;
    return true;
}

void TreeHighlighter::fill_with_lexer_full(
    const std::vector<std::string> &lines,
    Language lang)
{
    resize_line_cache(lines.size());
    HighlightState state;
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        LineCache &lc = line_cache_[i];
        lc.state_in = state;
        lc.tokens = Syntax::highlight_line(lines[i], lang, state);
        lc.state_out = state;
        lc.valid = true;
        line_tokens[i] = lc.tokens;
    }
    ts_active = false;
    dirty_from_ = static_cast<int>(lines.size());
}

void TreeHighlighter::rehighlight_lexer_from(
    const std::vector<std::string> &lines,
    Language lang,
    int from_line)
{
    resize_line_cache(lines.size());
    if (from_line < 0)
        from_line = 0;
    if (from_line > static_cast<int>(lines.size()))
        from_line = static_cast<int>(lines.size());

    HighlightState state;
    if (from_line > 0 && line_cache_[static_cast<std::size_t>(from_line - 1)].valid)
        state = line_cache_[static_cast<std::size_t>(from_line - 1)].state_out;

    for (int i = from_line; i < static_cast<int>(lines.size()); ++i)
    {
        LineCache &lc = line_cache_[static_cast<std::size_t>(i)];
        const HighlightState old_out = lc.state_out;
        const bool had_valid = lc.valid;

        lc.state_in = state;
        lc.tokens = Syntax::highlight_line(lines[static_cast<std::size_t>(i)], lang, state);
        lc.state_out = state;
        lc.valid = true;
        line_tokens[static_cast<std::size_t>(i)] = lc.tokens;

        // Stop when carry-out state matches previous cache (multiline state stable).
        if (had_valid && state == old_out)
        {
            dirty_from_ = i + 1;
            return;
        }
    }
    dirty_from_ = static_cast<int>(lines.size());
}

