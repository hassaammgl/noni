#pragma once
#include <syntax/syntax.hpp>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace syntax_lex
{
    bool is_ident_start(char c);
    bool is_ident_cont(char c);
    bool is_digit(char c);
    void push_token(std::vector<SyntaxToken> &out, int start, int end, TokenKind kind);

    const std::unordered_set<std::string> &cpp_keywords();
    const std::unordered_set<std::string> &c_keywords();
    const std::unordered_set<std::string> &python_keywords();
    const std::unordered_set<std::string> &rust_keywords();
    const std::unordered_set<std::string> &js_keywords();
    const std::unordered_set<std::string> &go_keywords();
    const std::unordered_set<std::string> &java_keywords();
    const std::unordered_set<std::string> &csharp_keywords();
    const std::unordered_set<std::string> &lua_keywords();
    const std::unordered_set<std::string> &shell_keywords();

    bool is_control(Language lang, const std::string &word);
    const std::unordered_set<std::string> *keywords_for(Language lang);
    bool uses_c_comments(Language lang);
    bool uses_hash_comments(Language lang);
    bool uses_lua_comments(Language lang);

    std::vector<SyntaxToken> highlight_markdown(std::string_view line, HighlightState &state);
    std::vector<SyntaxToken> highlight_generic(
        std::string_view line, Language lang, HighlightState &state);

    struct Scan
    {
        std::string_view line;
        Language lang;
        HighlightState &state;
        std::vector<SyntaxToken> &out;
        int i = 0;
        int n = 0;
        bool line_comment_end = false;
        const std::unordered_set<std::string> *kw = nullptr;

        char peek(int off = 0) const
        {
            const int j = i + off;
            return (j >= 0 && j < n) ? line[static_cast<std::size_t>(j)] : '\0';
        }

        bool resume_multiline();
        bool consume_comment();
        bool consume_string();
        bool consume_number();
        bool consume_ident();
        bool consume_operator();
        void scan_tokens();
    };
}
