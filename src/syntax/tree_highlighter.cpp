#include <syntax/tree_highlighter.hpp>
#include <syntax/grammar_installer.hpp>
#include <utils/logger.hpp>

#include <tree_sitter/api.h>

#include <dlfcn.h>
#include <algorithm>
#include <cstring>
#include <format>

const std::vector<SyntaxToken> TreeHighlighter::kEmpty{};

namespace
{
    using LangFn = const TSLanguage *(*)();

    struct LangSpec
    {
        const char *so_name;
        const char *symbol;
        const char *query;
    };

    // Minimal but useful highlight queries (tree-sitter capture names).
    // Keep queries conservative — invalid node names fail the whole query.
    constexpr const char *kQueryC = R"TS(
(comment) @comment
(string_literal) @string
(system_lib_string) @string
(char_literal) @string
(number_literal) @number
(primitive_type) @type
(type_identifier) @type
(preproc_include) @preprocessor
(preproc_def) @preprocessor
(preproc_function_def) @preprocessor
(preproc_call) @preprocessor
[
  "break" "case" "const" "continue" "default" "do" "else" "enum"
  "extern" "for" "if" "inline" "return" "sizeof" "static" "struct"
  "switch" "typedef" "union" "volatile" "while" "goto" "restrict"
] @keyword
(call_expression
  function: (identifier) @function)
(function_declarator
  declarator: (identifier) @function)
)TS";

    constexpr const char *kQueryLua = R"TS(
(comment) @comment
(string) @string
(number) @number
(true) @constant
(false) @constant
(nil) @constant
[
  "and" "do" "else" "elseif" "end" "for" "function"
  "goto" "if" "in" "local" "not" "or" "repeat" "return"
  "then" "until" "while"
] @keyword
(function_call
  name: (identifier) @function)
(function_declaration
  name: (identifier) @function)
)TS";

    constexpr const char *kQueryMarkdown = R"TS(
(atx_heading) @keyword
(setext_heading) @keyword
(fenced_code_block) @string
(indented_code_block) @string
(link_destination) @string
(link_title) @string
)TS";

    constexpr const char *kQueryPython = R"TS(
(comment) @comment
(string) @string
(escape_sequence) @string
(integer) @number
(float) @number
(true) @constant
(false) @constant
(none) @constant
[
  "and" "as" "assert" "async" "await" "break" "class" "continue"
  "def" "del" "elif" "else" "except" "finally" "for" "from"
  "global" "if" "import" "in" "is" "lambda" "nonlocal" "not"
  "or" "pass" "raise" "return" "try" "while" "with" "yield"
  "match" "case"
] @keyword
(function_definition
  name: (identifier) @function)
(call
  function: (identifier) @function)
(type) @type
)TS";

    constexpr const char *kQueryJS = R"TS(
(comment) @comment
(string) @string
(template_string) @string
(number) @number
(true) @constant
(false) @constant
(null) @constant
(undefined) @constant
[
  "as" "async" "await" "break" "case" "catch" "class" "const"
  "continue" "debugger" "default" "delete" "do" "else" "export"
  "extends" "finally" "for" "from" "function" "get" "if" "import"
  "in" "instanceof" "let" "new" "of" "return" "set" "static"
  "switch" "throw" "try" "typeof" "var" "void" "while" "with" "yield"
] @keyword
(call_expression
  function: (identifier) @function)
(function_declaration
  name: (identifier) @function)
(method_definition
  name: (property_identifier) @function)
)TS";

    constexpr const char *kQueryRust = R"TS(
(line_comment) @comment
(block_comment) @comment
(string_literal) @string
(raw_string_literal) @string
(integer_literal) @number
(float_literal) @number
(boolean_literal) @constant
[
  "as" "async" "await" "break" "const" "continue" "crate" "dyn"
  "else" "enum" "extern" "fn" "for" "if" "impl" "in" "let"
  "loop" "match" "mod" "move" "mut" "pub" "ref" "return" "self"
  "Self" "static" "struct" "super" "trait" "type" "unsafe" "use"
  "where" "while"
] @keyword
(call_expression
  function: (identifier) @function)
(function_item
  name: (identifier) @function)
(type_identifier) @type
(primitive_type) @type
)TS";

    constexpr const char *kQueryBash = R"TS(
(comment) @comment
(string) @string
(raw_string) @string
(number) @number
[
  "if" "then" "else" "elif" "fi" "case" "esac" "for" "select"
  "while" "until" "do" "done" "in" "function" "time" "coproc"
] @keyword
(command_name) @function
)TS";

    constexpr const char *kQueryJson = R"TS(
(comment) @comment
(string) @string
(number) @number
(true) @constant
(false) @constant
(null) @constant
)TS";

    const LangSpec *spec_for(Language lang)
    {
        static const LangSpec c{ "c.so", "tree_sitter_c", kQueryC };
        static const LangSpec lua{ "lua.so", "tree_sitter_lua", kQueryLua };
        static const LangSpec md{ "markdown.so", "tree_sitter_markdown", kQueryMarkdown };
        static const LangSpec py{ "python.so", "tree_sitter_python", kQueryPython };
        static const LangSpec js{ "javascript.so", "tree_sitter_javascript", kQueryJS };
        static const LangSpec rs{ "rust.so", "tree_sitter_rust", kQueryRust };
        static const LangSpec sh{ "bash.so", "tree_sitter_bash", kQueryBash };
        static const LangSpec json{ "json.so", "tree_sitter_json", kQueryJson };
        static const LangSpec cpp{ "cpp.so", "tree_sitter_cpp", kQueryC }; // same-ish captures

        switch (lang)
        {
        case Language::C:
            return &c;
        case Language::Cpp:
            return &cpp; // may fall back to c in ensure_language
        case Language::Lua:
            return &lua;
        case Language::Markdown:
            return &md;
        case Language::Python:
            return &py;
        case Language::JavaScript:
        case Language::TypeScript:
            return &js;
        case Language::Rust:
            return &rs;
        case Language::Shell:
            return &sh;
        case Language::JSON:
            return &json;
        default:
            return nullptr;
        }
    }

    TokenKind capture_to_kind(std::string_view name)
    {
        if (name.find("comment") != std::string_view::npos)
            return TokenKind::Comment;
        if (name.find("string") != std::string_view::npos)
            return TokenKind::String;
        if (name.find("number") != std::string_view::npos)
            return TokenKind::Number;
        if (name.find("constant") != std::string_view::npos ||
            name.find("boolean") != std::string_view::npos)
            return TokenKind::Constant;
        if (name.find("function") != std::string_view::npos)
            return TokenKind::Function;
        if (name.find("type") != std::string_view::npos)
            return TokenKind::Type;
        if (name.find("preprocessor") != std::string_view::npos ||
            name.find("directive") != std::string_view::npos)
            return TokenKind::Preprocessor;
        if (name.find("keyword") != std::string_view::npos ||
            name.find("conditional") != std::string_view::npos ||
            name.find("repeat") != std::string_view::npos)
            return TokenKind::Keyword;
        if (name.find("operator") != std::string_view::npos)
            return TokenKind::Operator;
        if (name.find("punctuation") != std::string_view::npos)
            return TokenKind::Punctuation;
        return TokenKind::Text;
    }

    void add_span(
        std::vector<std::vector<SyntaxToken>> &lines,
        uint32_t start_row,
        uint32_t start_col,
        uint32_t end_row,
        uint32_t end_col,
        TokenKind kind)
    {
        if (lines.empty())
            return;
        if (start_row >= lines.size())
            return;

        if (start_row == end_row)
        {
            if (end_col > start_col)
                lines[start_row].push_back(SyntaxToken{
                    static_cast<int>(start_col),
                    static_cast<int>(end_col - start_col),
                    kind });
            return;
        }

        lines[start_row].push_back(SyntaxToken{
            static_cast<int>(start_col), 100000, kind });
        for (uint32_t r = start_row + 1; r < end_row && r < lines.size(); ++r)
            lines[r].push_back(SyntaxToken{0, 100000, kind});
        if (end_row < lines.size() && end_col > 0)
            lines[end_row].push_back(SyntaxToken{0, static_cast<int>(end_col), kind});
    }

    std::string join_source(const std::vector<std::string> &lines)
    {
        std::string out;
        std::size_t bytes = 0;
        for (const auto &l : lines)
            bytes += l.size() + 1;
        out.reserve(bytes);
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            out += lines[i];
            if (i + 1 < lines.size())
                out.push_back('\n');
        }
        return out;
    }
}

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
