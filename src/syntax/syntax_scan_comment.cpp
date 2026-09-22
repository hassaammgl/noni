#include "syntax_lex.hpp"

namespace syntax_lex
{
    bool Scan::consume_comment()
    {
        const char c = peek();

        if (c == '#' && uses_hash_comments(lang))
        {
            push_token(out, i, n, TokenKind::Comment);
            line_comment_end = true;
            return true;
        }

        if (uses_lua_comments(lang) && c == '-' && peek(1) == '-')
        {
            push_token(out, i, n, TokenKind::Comment);
            line_comment_end = true;
            return true;
        }

        if (uses_c_comments(lang) && c == '/' && peek(1) == '/')
        {
            push_token(out, i, n, TokenKind::Comment);
            line_comment_end = true;
            return true;
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
            return true;
        }
        return false;
    }

    bool Scan::consume_string()
    {
        const char c = peek();
        if (c == '"' || c == '\'' || (c == '`' && (lang == Language::JavaScript ||
                                                   lang == Language::TypeScript ||
                                                   lang == Language::Shell)))
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
            return true;
        }

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
            return true;
        }
        return false;
    }
}
