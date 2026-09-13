#pragma once

#include <editor/undo.hpp>
#include <syntax/syntax.hpp>

#include <cstdint>
#include <string>
#include <vector>

// Per-buffer syntax engine: tree-sitter (incremental) with lexer fallback.
// Tokens are logical byte spans; rendering converts to display columns.
class TreeHighlighter
{
public:
    TreeHighlighter();
    ~TreeHighlighter();

    TreeHighlighter(const TreeHighlighter &) = delete;
    TreeHighlighter &operator=(const TreeHighlighter &) = delete;

    // Called after Buffer applies a TextChange (content already updated).
    void notify_edit(const TextChange &change);

    // Full invalidate (load, path change, undo/redo of multi-change txn).
    void invalidate_all();

    // Bring cache up to date for this document. Cheap when clean.
    void sync(
        const fs::path &path,
        const std::vector<std::string> &lines,
        std::uint64_t revision);

    const std::vector<SyntaxToken> &tokens_for_line(int line) const;
    bool using_tree_sitter() const;
    Language language() const { return cached_lang; }

private:
    struct LineCache
    {
        HighlightState state_in{};
        HighlightState state_out{};
        std::vector<SyntaxToken> tokens;
        bool valid = false;
    };

    void clear_ts();
    bool ensure_language(Language lang);
    bool parse_with_ts(
        const std::string &source,
        Language lang,
        bool incremental,
        std::size_t line_count);
    void fill_with_lexer_full(const std::vector<std::string> &lines, Language lang);
    void rehighlight_lexer_from(
        const std::vector<std::string> &lines,
        Language lang,
        int from_line);
    void apply_query();
    void clip_tokens(const std::vector<std::string> &lines);
    void resize_line_cache(std::size_t n);
    static void advance_point(int &line, int &col, std::string_view text);
    static std::uint32_t byte_offset(
        const std::vector<std::string> &lines,
        int line,
        int col);

    bool needs_sync_ = true;
    bool force_full_ = true;
    int dirty_from_ = 0;

    std::uint64_t cached_revision = 0;
    std::uint64_t cached_grammar_gen = 0;
    Language cached_lang = Language::Plain;
    Language loaded_lang = Language::Plain;
    fs::path cached_path;
    bool ts_active = false;

    void *dl_handle = nullptr;
    const void *ts_language = nullptr; // TSLanguage*
    void *parser = nullptr;            // TSParser*
    void *query = nullptr;             // TSQuery*
    void *tree = nullptr;              // TSTree*

    std::string cached_source;
    std::vector<TextChange> pending_edits_;
    std::vector<LineCache> line_cache_;
    std::vector<std::vector<SyntaxToken>> line_tokens; // TS path / mirror

    static const std::vector<SyntaxToken> kEmpty;
};
