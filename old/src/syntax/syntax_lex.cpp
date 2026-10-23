#include "syntax_lex.hpp"
#include <cctype>

namespace syntax_lex
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
}
