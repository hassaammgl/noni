#include <syntax/syntax.hpp>
#include "syntax_lex.hpp"
#include <ui/theme.hpp>

#include <cctype>

using namespace syntax_lex;

Language Syntax::detect_language(const fs::path &path)
{
    std::string ext = path.extension().string();
    for (char &c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    const std::string name = path.filename().string();

    if (ext == ".c" || ext == ".h")
        return Language::C;
    if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".hpp" ||
        ext == ".hh" || ext == ".hxx" || ext == ".ipp" || ext == ".tpp")
        return Language::Cpp;
    if (ext == ".py" || ext == ".pyw" || ext == ".pyi")
        return Language::Python;
    if (ext == ".rs")
        return Language::Rust;
    if (ext == ".go")
        return Language::Go;
    if (ext == ".js" || ext == ".mjs" || ext == ".cjs" || ext == ".jsx")
        return Language::JavaScript;
    if (ext == ".ts" || ext == ".tsx")
        return Language::TypeScript;
    if (ext == ".json" || ext == ".jsonc")
        return Language::JSON;
    if (ext == ".sh" || ext == ".bash" || ext == ".zsh" || name == "Makefile" ||
        name == "makefile" || name == "Dockerfile")
        return Language::Shell;
    if (ext == ".lua")
        return Language::Lua;
    if (ext == ".java")
        return Language::Java;
    if (ext == ".cs")
        return Language::CSharp;
    if (ext == ".md" || ext == ".markdown")
        return Language::Markdown;

    return Language::Plain;
}

const LanguageDefinition &Syntax::language_definition(Language lang)
{
    static const LanguageDefinition plain{Language::Plain, "plain", false, false, false};
    static const LanguageDefinition c{Language::C, "c", true, true, true};
    static const LanguageDefinition cpp{Language::Cpp, "cpp", true, true, true};
    static const LanguageDefinition python{Language::Python, "python", true, false, true};
    static const LanguageDefinition rust{Language::Rust, "rust", true, true, true};
    static const LanguageDefinition go{Language::Go, "go", true, true, true};
    static const LanguageDefinition js{Language::JavaScript, "javascript", true, true, true};
    static const LanguageDefinition ts{Language::TypeScript, "typescript", true, true, true};
    static const LanguageDefinition json{Language::JSON, "json", false, false, true};
    static const LanguageDefinition shell{Language::Shell, "shell", true, false, true};
    static const LanguageDefinition lua{Language::Lua, "lua", true, true, true};
    static const LanguageDefinition java{Language::Java, "java", true, true, true};
    static const LanguageDefinition csharp{Language::CSharp, "csharp", true, true, true};
    static const LanguageDefinition markdown{Language::Markdown, "markdown", false, false, true};

    switch (lang)
    {
    case Language::C:
        return c;
    case Language::Cpp:
        return cpp;
    case Language::Python:
        return python;
    case Language::Rust:
        return rust;
    case Language::Go:
        return go;
    case Language::JavaScript:
        return js;
    case Language::TypeScript:
        return ts;
    case Language::JSON:
        return json;
    case Language::Shell:
        return shell;
    case Language::Lua:
        return lua;
    case Language::Java:
        return java;
    case Language::CSharp:
        return csharp;
    case Language::Markdown:
        return markdown;
    case Language::Plain:
    default:
        return plain;
    }
}

std::vector<SyntaxToken> Syntax::highlight_line(
    std::string_view line,
    Language lang,
    HighlightState &state)
{
    if (lang == Language::Plain)
    {
        if (!line.empty())
            return {SyntaxToken{0, static_cast<int>(line.size()), TokenKind::Text}};
        return {};
    }
    if (lang == Language::Markdown)
        return highlight_markdown(line, state);

    // Python triple-quote continuation
    if (lang == Language::Python && state.in_string && state.string_rawish)
    {
        std::vector<SyntaxToken> out;
        const int n = static_cast<int>(line.size());
        int i = 0;
        const char delim = state.string_delim;
        while (i + 2 < n)
        {
            if (line[static_cast<std::size_t>(i)] == delim &&
                line[static_cast<std::size_t>(i + 1)] == delim &&
                line[static_cast<std::size_t>(i + 2)] == delim)
            {
                i += 3;
                state.in_string = false;
                state.string_rawish = false;
                state.string_delim = 0;
                push_token(out, 0, i, TokenKind::String);
                HighlightState rest;
                auto more = highlight_generic(line.substr(static_cast<std::size_t>(i)), lang, rest);
                state = rest;
                for (auto &t : more)
                {
                    t.start += i;
                    out.push_back(t);
                }
                return out;
            }
            ++i;
        }
        push_token(out, 0, n, TokenKind::String);
        return out;
    }

    return highlight_generic(line, lang, state);
}

short Syntax::theme_pair(TokenKind kind)
{
    switch (kind)
    {
    case TokenKind::Comment:
        return Theme::Comment;
    case TokenKind::CommentDoc:
        return Theme::CommentDoc;
    case TokenKind::Keyword:
        return Theme::Keyword;
    case TokenKind::KeywordControl:
        return Theme::KeywordControl;
    case TokenKind::Storage:
        return Theme::Storage;
    case TokenKind::Type:
        return Theme::Type;
    case TokenKind::String:
        return Theme::String;
    case TokenKind::StringEscape:
        return Theme::StringEscape;
    case TokenKind::Number:
        return Theme::Number;
    case TokenKind::Constant:
        return Theme::Constant;
    case TokenKind::Function:
        return Theme::Function;
    case TokenKind::Macro:
        return Theme::Macro;
    case TokenKind::Operator:
        return Theme::Operator;
    case TokenKind::Preprocessor:
        return Theme::Preprocessor;
    case TokenKind::Punctuation:
        return Theme::Punctuation;
    case TokenKind::Text:
    default:
        return Theme::Editor;
    }
}
