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

const std::vector<SyntaxToken> TreeHighlighter::kEmpty{};

TreeHighlighter::TreeHighlighter() = default;

TreeHighlighter::~TreeHighlighter()
{
    clear_ts();
}

void TreeHighlighter::clear_ts()
{
    if (tree)
    {
        ts_tree_delete(static_cast<TSTree *>(tree));
        tree = nullptr;
    }
    if (query)
    {
        ts_query_delete(static_cast<TSQuery *>(query));
        query = nullptr;
    }
    if (parser)
    {
        ts_parser_delete(static_cast<TSParser *>(parser));
        parser = nullptr;
    }
    ts_language = nullptr;
    loaded_lang = Language::Plain;
    if (dl_handle)
    {
        dlclose(dl_handle);
        dl_handle = nullptr;
    }
    ts_active = false;
    cached_source.clear();
}

bool TreeHighlighter::using_tree_sitter() const
{
    return ts_active;
}

void TreeHighlighter::advance_point(int &line, int &col, std::string_view text)
{
    for (char ch : text)
    {
        if (ch == '\n')
        {
            ++line;
            col = 0;
        }
        else
        {
            ++col;
        }
    }
}

std::uint32_t TreeHighlighter::byte_offset(
    const std::vector<std::string> &lines,
    int line,
    int col)
{
    if (line < 0)
        return 0;
    std::uint32_t off = 0;
    const int n = static_cast<int>(lines.size());
    for (int i = 0; i < line && i < n; ++i)
        off += static_cast<std::uint32_t>(lines[static_cast<std::size_t>(i)].size()) + 1;
    if (line >= n)
        return off;
    const int lim = static_cast<int>(lines[static_cast<std::size_t>(line)].size());
    off += static_cast<std::uint32_t>(std::clamp(col, 0, lim));
    return off;
}

void TreeHighlighter::notify_edit(const TextChange &change)
{
    needs_sync_ = true;
    pending_edits_.push_back(change);
    if (change.start_line < dirty_from_)
        dirty_from_ = change.start_line;
    if (dirty_from_ < 0)
        dirty_from_ = 0;
}

void TreeHighlighter::invalidate_all()
{
    needs_sync_ = true;
    force_full_ = true;
    dirty_from_ = 0;
    pending_edits_.clear();
    cached_revision = 0;
    for (auto &lc : line_cache_)
        lc.valid = false;
    if (tree)
    {
        ts_tree_delete(static_cast<TSTree *>(tree));
        tree = nullptr;
    }
    cached_source.clear();
    ts_active = false;
}

void TreeHighlighter::resize_line_cache(std::size_t n)
{
    if (line_cache_.size() == n && line_tokens.size() == n)
        return;
    line_cache_.resize(n);
    line_tokens.resize(n);
}

