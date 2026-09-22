#include "syntax_lex.hpp"

namespace syntax_lex
{
    bool Scan::resume_multiline()
    {
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
                return true;
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
                return true;
            }
        }
        return false;
    }
}
