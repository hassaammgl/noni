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

struct SyntaxToken
{
    int start = 0; // column in line
    int length = 0;
    TokenKind kind = TokenKind::Text;
};

struct HighlightState
{
    bool in_block_comment = false;
    bool in_string = false;
    char string_delim = 0;
    bool string_rawish = false; // for simple continuation
};

namespace Syntax
{
    Language detect_language(const fs::path &path);

    // Advances state; returns tokens for this line.
    std::vector<SyntaxToken> highlight_line(
        std::string_view line,
        Language lang,
        HighlightState &state);

    // Map token → Theme color pair id.
    short theme_pair(TokenKind kind);
}
