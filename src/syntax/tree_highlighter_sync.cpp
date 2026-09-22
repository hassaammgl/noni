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

void TreeHighlighter::sync(
    const fs::path &path,
    const std::vector<std::string> &lines,
    std::uint64_t revision)
{
    const Language lang = Syntax::detect_language(path);
    (void)Syntax::language_definition(lang);
    const std::uint64_t grammar_gen = GrammarInstaller::generation();

    const bool meta_changed =
        lang != cached_lang ||
        path != cached_path ||
        grammar_gen != cached_grammar_gen;

    if (!needs_sync_ &&
        !force_full_ &&
        !meta_changed &&
        revision == cached_revision &&
        line_tokens.size() == lines.size())
        return;

    if (meta_changed)
        force_full_ = true;

    cached_lang = lang;
    cached_path = path;
    cached_grammar_gen = grammar_gen;
    cached_revision = revision;
    needs_sync_ = false;

    resize_line_cache(lines.size());

    if (lang == Language::Plain || lines.empty())
    {
        pending_edits_.clear();
        force_full_ = false;
        fill_with_lexer_full(lines, lang);
        return;
    }

    const std::string source = join_source(lines);
    bool used_ts = false;

    if (!force_full_ && ts_active && tree && !pending_edits_.empty() && ensure_language(lang))
    {
        // Apply pending edits to the old tree using positions in the post-edit document.
        // start_byte is identical before/after; deleted/inserted sizes give old/new ends.
        bool edits_ok = true;
        for (const TextChange &ch : pending_edits_)
        {
            const std::uint32_t start_byte = byte_offset(lines, ch.start_line, ch.start_col);
            const std::uint32_t old_end_byte =
                start_byte + static_cast<std::uint32_t>(ch.deleted.size());
            const std::uint32_t new_end_byte =
                start_byte + static_cast<std::uint32_t>(ch.inserted.size());

            int ol = ch.start_line;
            int oc = ch.start_col;
            advance_point(ol, oc, ch.deleted);
            int nl = ch.start_line;
            int nc = ch.start_col;
            advance_point(nl, nc, ch.inserted);

            TSInputEdit edit{};
            edit.start_byte = start_byte;
            edit.old_end_byte = old_end_byte;
            edit.new_end_byte = new_end_byte;
            edit.start_point = {
                static_cast<uint32_t>(ch.start_line),
                static_cast<uint32_t>(ch.start_col)};
            edit.old_end_point = {
                static_cast<uint32_t>(ol),
                static_cast<uint32_t>(oc)};
            edit.new_end_point = {
                static_cast<uint32_t>(nl),
                static_cast<uint32_t>(nc)};
            ts_tree_edit(static_cast<TSTree *>(tree), &edit);
        }

        pending_edits_.clear();
        if (edits_ok && parse_with_ts(source, lang, true, lines.size()))
        {
            used_ts = true;
            clip_tokens(lines);
        }
    }
    else
    {
        pending_edits_.clear();
        if (parse_with_ts(source, lang, false, lines.size()))
        {
            used_ts = true;
            clip_tokens(lines);
        }
    }

    force_full_ = false;

    if (used_ts)
    {
        // Mirror into line_cache for consumers that only read tokens_for_line.
        for (std::size_t i = 0; i < lines.size() && i < line_tokens.size(); ++i)
        {
            line_cache_[i].tokens = line_tokens[i];
            line_cache_[i].valid = true;
        }
        dirty_from_ = static_cast<int>(lines.size());
        return;
    }

    Logger::debug(std::format(
        "tree-sitter unavailable for {}, using incremental lexer",
        path.filename().string()));

    if (dirty_from_ >= static_cast<int>(lines.size()))
        dirty_from_ = 0;
    rehighlight_lexer_from(lines, lang, dirty_from_);
}

const std::vector<SyntaxToken> &TreeHighlighter::tokens_for_line(int line) const
{
    if (line < 0 || line >= static_cast<int>(line_tokens.size()))
        return kEmpty;
    return line_tokens[static_cast<std::size_t>(line)];
}
