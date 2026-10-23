#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

enum class Language
{
    Plain,
    Cpp,
    C,
    Python,
    Rust,
    Go,
    JavaScript,
    TypeScript,
    JSON,
    Shell,
    Lua,
    Java,
    CSharp,
    Markdown,
};

enum class TokenKind : std::uint8_t
{
    Text,
    Comment,
    CommentDoc,
    Keyword,
    KeywordControl,
    Storage,
    Type,
    String,
    StringEscape,
    Number,
    Constant,
    Function,
    Macro,
    Operator,
    Preprocessor,
    Punctuation,
};

// Logical span within a line: start/length are BYTE offsets into the UTF-8 line.
struct SyntaxToken
{
    int start = 0;
    int length = 0;
    TokenKind kind = TokenKind::Text;
};

struct HighlightState
{
    bool in_block_comment = false;
    bool in_string = false;
    char string_delim = 0;
    bool string_rawish = false; // for simple continuation

    bool operator==(const HighlightState &o) const
    {
        return in_block_comment == o.in_block_comment &&
               in_string == o.in_string &&
               string_delim == o.string_delim &&
               string_rawish == o.string_rawish;
    }

    bool operator!=(const HighlightState &o) const { return !(*this == o); }
};

// Lightweight language metadata (not a plugin framework).
struct LanguageDefinition
{
    Language id = Language::Plain;
    const char *name = "plain";
    bool line_comments = false;
    bool block_comments = false;
    bool strings = false;
};

namespace Syntax
{
    Language detect_language(const fs::path &path);
    const LanguageDefinition &language_definition(Language lang);

    // Advances state; returns tokens for this line.
    std::vector<SyntaxToken> highlight_line(
        std::string_view line,
        Language lang,
        HighlightState &state);

    // Map token → Theme color pair id.
    short theme_pair(TokenKind kind);
}
