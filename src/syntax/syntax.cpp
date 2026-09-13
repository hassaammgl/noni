#include <syntax/syntax.hpp>
#include <ui/theme.hpp>

#include <cctype>
#include <unordered_set>

namespace
{
    bool is_ident_start(char c)
    {
        return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '$';
    }

    bool is_ident_cont(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '$';
    }

    bool is_digit(char c)
    {
        return std::isdigit(static_cast<unsigned char>(c));
    }

    void push_token(std::vector<SyntaxToken> &out, int start, int end, TokenKind kind)
    {
        if (end <= start)
            return;
        out.push_back(SyntaxToken{start, end - start, kind});
    }

    const std::unordered_set<std::string> &cpp_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
            "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
            "class", "compl", "concept", "const", "consteval", "constexpr", "constinit",
            "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype",
            "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
            "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
            "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
            "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected",
            "public", "register", "reinterpret_cast", "requires", "return", "short",
            "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
            "switch", "template", "this", "thread_local", "throw", "true", "try",
            "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual",
            "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
            "override", "final", "decltype",
        };
        return k;
    }

    const std::unordered_set<std::string> &c_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "auto", "break", "case", "char", "const", "continue", "default", "do",
            "double", "else", "enum", "extern", "float", "for", "goto", "if",
            "inline", "int", "long", "register", "restrict", "return", "short",
            "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
            "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary",
            "true", "false", "nullptr",
        };
        return k;
    }

    const std::unordered_set<std::string> &python_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "False", "None", "True", "and", "as", "assert", "async", "await",
            "break", "class", "continue", "def", "del", "elif", "else", "except",
            "finally", "for", "from", "global", "if", "import", "in", "is",
            "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
            "while", "with", "yield", "match", "case", "type",
        };
        return k;
    }

    const std::unordered_set<std::string> &rust_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "as", "async", "await", "break", "const", "continue", "crate", "dyn",
            "else", "enum", "extern", "false", "fn", "for", "if", "impl", "in",
            "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return",
            "self", "Self", "static", "struct", "super", "trait", "true", "type",
            "unsafe", "use", "where", "while", "async", "await", "dyn",
        };
        return k;
    }

    const std::unordered_set<std::string> &js_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "await", "break", "case", "catch", "class", "const", "continue", "debugger",
            "default", "delete", "do", "else", "enum", "export", "extends", "false",
            "finally", "for", "function", "if", "implements", "import", "in",
            "instanceof", "interface", "let", "new", "null", "package", "private",
            "protected", "public", "return", "static", "super", "switch", "this",
            "throw", "true", "try", "typeof", "var", "void", "while", "with", "yield",
            "async", "of", "from", "as", "type", "namespace",
        };
        return k;
    }

    const std::unordered_set<std::string> &go_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "break", "case", "chan", "const", "continue", "default", "defer", "else",
            "fallthrough", "for", "func", "go", "goto", "if", "import", "interface",
            "map", "package", "range", "return", "select", "struct", "switch", "type",
            "var", "true", "false", "nil",
        };
        return k;
    }

    const std::unordered_set<std::string> &java_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char",
            "class", "const", "continue", "default", "do", "double", "else", "enum",
            "extends", "final", "finally", "float", "for", "goto", "if", "implements",
            "import", "instanceof", "int", "interface", "long", "native", "new",
            "package", "private", "protected", "public", "return", "short", "static",
            "strictfp", "super", "switch", "synchronized", "this", "throw", "throws",
            "transient", "try", "void", "volatile", "while", "true", "false", "null",
            "var", "record", "sealed", "permits", "yield",
        };
        return k;
    }

    const std::unordered_set<std::string> &csharp_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char",
            "checked", "class", "const", "continue", "decimal", "default", "delegate",
            "do", "double", "else", "enum", "event", "explicit", "extern", "false",
            "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit",
            "in", "int", "interface", "internal", "is", "lock", "long", "namespace",
            "new", "null", "object", "operator", "out", "override", "params", "private",
            "protected", "public", "readonly", "ref", "return", "sbyte", "sealed",
            "short", "sizeof", "stackalloc", "static", "string", "struct", "switch",
            "this", "throw", "true", "try", "typeof", "uint", "ulong", "unchecked",
            "unsafe", "ushort", "using", "virtual", "void", "volatile", "while",
            "async", "await", "var", "record", "init", "required",
        };
        return k;
    }

    const std::unordered_set<std::string> &lua_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "and", "break", "do", "else", "elseif", "end", "false", "for", "function",
            "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return",
            "then", "true", "until", "while",
        };
        return k;
    }

    const std::unordered_set<std::string> &shell_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "if", "then", "else", "elif", "fi", "case", "esac", "for", "select",
            "while", "until", "do", "done", "in", "function", "time", "coproc",
            "return", "exit", "break", "continue", "export", "local", "readonly",
            "declare", "typeset", "unset", "shift", "source",
        };
        return k;
    }

    bool is_control(Language lang, const std::string &word)
    {
        static const std::unordered_set<std::string> ctrl = {
            "if", "else", "elif", "elseif", "for", "while", "do", "switch", "case",
            "default", "break", "continue", "return", "goto", "try", "catch", "throw",
            "finally", "except", "raise", "yield", "await", "match", "loop", "when",
            "fi", "done", "esac", "then", "until", "select",
        };
        (void)lang;
        return ctrl.count(word) > 0;
    }

    const std::unordered_set<std::string> *keywords_for(Language lang)
    {
        switch (lang)
        {
        case Language::Cpp:
            return &cpp_keywords();
        case Language::C:
            return &c_keywords();
        case Language::Python:
            return &python_keywords();
        case Language::Rust:
            return &rust_keywords();
        case Language::JavaScript:
        case Language::TypeScript:
            return &js_keywords();
        case Language::Go:
            return &go_keywords();
        case Language::Java:
            return &java_keywords();
        case Language::CSharp:
            return &csharp_keywords();
        case Language::Lua:
            return &lua_keywords();
        case Language::Shell:
            return &shell_keywords();
        default:
            return nullptr;
        }
    }

    bool uses_c_comments(Language lang)
    {
        switch (lang)
        {
        case Language::Cpp:
        case Language::C:
        case Language::Rust:
        case Language::Go:
        case Language::JavaScript:
        case Language::TypeScript:
        case Language::Java:
        case Language::CSharp:
        case Language::JSON:
            return true;
        default:
            return false;
        }
    }

    bool uses_hash_comments(Language lang)
    {
        return lang == Language::Python || lang == Language::Shell;
    }

    bool uses_lua_comments(Language lang)
    {
        return lang == Language::Lua;
    }

    std::vector<SyntaxToken> highlight_markdown(std::string_view line, HighlightState &)
    {
        std::vector<SyntaxToken> out;
        if (line.empty())
            return out;

        if (line[0] == '#')
        {
            push_token(out, 0, static_cast<int>(line.size()), TokenKind::Keyword);
            return out;
        }
        // inline `code`
        for (std::size_t i = 0; i < line.size();)
        {
            if (line[i] == '`')
            {
                const std::size_t start = i++;
                while (i < line.size() && line[i] != '`')
                    ++i;
                if (i < line.size())
                    ++i;
                push_token(out, static_cast<int>(start), static_cast<int>(i), TokenKind::String);
                continue;
            }
            ++i;
        }
        if (out.empty())
            push_token(out, 0, static_cast<int>(line.size()), TokenKind::Text);
        return out;
    }

    std::vector<SyntaxToken> highlight_generic(
        std::string_view line,
        Language lang,
        HighlightState &state)
    {
        std::vector<SyntaxToken> out;
        const int n = static_cast<int>(line.size());
        int i = 0;

        auto peek = [&](int off = 0) -> char {
            const int j = i + off;
            return (j >= 0 && j < n) ? line[static_cast<std::size_t>(j)] : '\0';
        };

        // Continue block comment / string from previous line
        if (state.in_block_comment)
        {
            const int start = 0;
            while (i < n)
            {
                if (peek() == '*' && peek(1) == '/')
                {
                    i += 2;
                    state.in_block_comment = false;
                    push_token(out, start, i, TokenKind::Comment);
                    break;
                }
                ++i;
            }
            if (state.in_block_comment)
            {
                push_token(out, start, n, TokenKind::Comment);
                return out;
            }
        }

        if (state.in_string)
        {
            const int start = 0;
            const char delim = state.string_delim;
            while (i < n)
            {
                if (peek() == '\\' && i + 1 < n)
                {
                    push_token(out, i, i + 2, TokenKind::StringEscape);
                    i += 2;
                    continue;
                }
                if (peek() == delim)
                {
                    ++i;
                    state.in_string = false;
                    state.string_delim = 0;
                    push_token(out, start, i, TokenKind::String);
                    break;
                }
                ++i;
            }
            if (state.in_string)
            {
                push_token(out, start, n, TokenKind::String);
                return out;
            }
        }

        const auto *kw = keywords_for(lang);

        while (i < n)
        {
            const char c = peek();

            // whitespace
            if (c == ' ' || c == '\t')
            {
                ++i;
                continue;
            }

            // preprocessor
            if (c == '#' && (lang == Language::Cpp || lang == Language::C) && i == 0)
            {
                push_token(out, 0, n, TokenKind::Preprocessor);
                return out;
            }

            // hash comments
            if (c == '#' && uses_hash_comments(lang))
            {
                push_token(out, i, n, TokenKind::Comment);
                break;
            }

            // lua comments
            if (uses_lua_comments(lang) && c == '-' && peek(1) == '-')
            {
                push_token(out, i, n, TokenKind::Comment);
                break;
            }

            // C-style comments
            if (uses_c_comments(lang) && c == '/' && peek(1) == '/')
            {
                push_token(out, i, n, TokenKind::Comment);
                break;
            }
            if (uses_c_comments(lang) && c == '/' && peek(1) == '*')
            {
                const int start = i;
                i += 2;
                state.in_block_comment = true;
                while (i < n)
                {
                    if (peek() == '*' && peek(1) == '/')
                    {
                        i += 2;
                        state.in_block_comment = false;
                        break;
                    }
                    ++i;
                }
                push_token(out, start, i, TokenKind::Comment);
                continue;
            }

            // strings
            if (c == '"' || c == '\'' || (c == '`' && (lang == Language::JavaScript || lang == Language::TypeScript || lang == Language::Shell)))
            {
                const int start = i;
                const char delim = c;
                ++i;
                bool closed = false;
                while (i < n)
                {
                    if (peek() == '\\' && i + 1 < n)
                    {
                        i += 2;
                        continue;
                    }
                    if (peek() == delim)
                    {
                        ++i;
                        closed = true;
                        break;
                    }
                    ++i;
                }
                push_token(out, start, i, TokenKind::String);
                if (!closed && delim != '\'') // allow multi-line for " and `
                {
                    state.in_string = true;
                    state.string_delim = delim;
                }
                continue;
            }

            // python triple quotes start
            if (lang == Language::Python &&
                ((c == '"' && peek(1) == '"' && peek(2) == '"') ||
                 (c == '\'' && peek(1) == '\'' && peek(2) == '\'')))
            {
                const int start = i;
                const char delim = c;
                i += 3;
                bool closed = false;
                while (i + 2 < n)
                {
                    if (peek() == delim && peek(1) == delim && peek(2) == delim)
                    {
                        i += 3;
                        closed = true;
                        break;
                    }
                    ++i;
                }
                if (!closed)
                {
                    i = n;
                    state.in_string = true;
                    state.string_delim = delim;
                    state.string_rawish = true; // mark triple — treated as string until triple close
                }
                push_token(out, start, i, TokenKind::String);
                continue;
            }

            // numbers
            if (is_digit(c) || (c == '.' && is_digit(peek(1))))
            {
                const int start = i;
                if (c == '0' && (peek(1) == 'x' || peek(1) == 'X'))
                {
                    i += 2;
                    while (i < n && std::isxdigit(static_cast<unsigned char>(peek())))
                        ++i;
                }
                else
                {
                    while (i < n && (is_digit(peek()) || peek() == '.' || peek() == '_' ||
                                     peek() == 'e' || peek() == 'E' || peek() == '+' || peek() == '-'))
                    {
                        if ((peek() == '+' || peek() == '-') &&
                            !(line[static_cast<std::size_t>(i - 1)] == 'e' ||
                              line[static_cast<std::size_t>(i - 1)] == 'E'))
                            break;
                        ++i;
                    }
                }
                while (i < n && (peek() == 'f' || peek() == 'F' || peek() == 'u' || peek() == 'U' ||
                                 peek() == 'l' || peek() == 'L'))
                    ++i;
                push_token(out, start, i, TokenKind::Number);
                continue;
            }

            // identifiers / keywords / function-ish
            if (is_ident_start(c))
            {
                const int start = i;
                ++i;
                while (i < n && is_ident_cont(peek()))
                    ++i;
                const std::string word(line.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(i - start)));

                TokenKind kind = TokenKind::Text;
                if (kw && kw->count(word))
                {
                    kind = is_control(lang, word) ? TokenKind::KeywordControl : TokenKind::Keyword;
                    if (word == "true" || word == "false" || word == "null" || word == "nullptr" ||
                        word == "None" || word == "True" || word == "False" || word == "nil")
                        kind = TokenKind::Constant;
                }
                else if (peek() == '(')
                {
                    kind = TokenKind::Function;
                }
                else if (!word.empty() && std::isupper(static_cast<unsigned char>(word[0])) &&
                         (lang == Language::Cpp || lang == Language::Java || lang == Language::CSharp ||
                          lang == Language::Rust || lang == Language::TypeScript))
                {
                    kind = TokenKind::Type;
                }

                push_token(out, start, i, kind);
                continue;
            }

            // operators / punctuation
            if (std::ispunct(static_cast<unsigned char>(c)))
            {
                const int start = i;
                ++i;
                // multi-char ops
                while (i < n)
                {
                    const char a = peek(-1);
                    const char b = peek();
                    const bool multi =
                        (a == '=' && (b == '=' || b == '>')) ||
                        (a == '!' && b == '=') ||
                        (a == '<' && (b == '=' || b == '<' || b == '>')) ||
                        (a == '>' && (b == '=' || b == '>')) ||
                        (a == '&' && b == '&') ||
                        (a == '|' && b == '|') ||
                        (a == '+' && b == '+') ||
                        (a == '-' && (b == '-' || b == '>')) ||
                        (a == ':' && b == ':') ||
                        (a == '.' && b == '.');
                    if (!multi)
                        break;
                    ++i;
                    if (i < n && peek(-1) == '.' && peek() == '.')
                        ++i; // ...
                }
                push_token(out, start, i, TokenKind::Operator);
                continue;
            }

            ++i;
        }

        return out;
    }
}

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
                // remainder with fresh state
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
