#include "ts_hl_detail.hpp"

namespace ts_hl_detail
{
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
