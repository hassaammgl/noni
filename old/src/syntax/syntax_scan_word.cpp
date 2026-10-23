#include "syntax_lex.hpp"
#include <cctype>

namespace syntax_lex
{
    bool Scan::consume_number()
    {
        const char c = peek();
        if (!(is_digit(c) || (c == '.' && is_digit(peek(1)))))
            return false;

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
        return true;
    }

    bool Scan::consume_ident()
    {
        const char c = peek();
        if (!is_ident_start(c))
            return false;

        const int start = i;
        ++i;
        while (i < n && is_ident_cont(peek()))
            ++i;
        const std::string word(line.substr(static_cast<std::size_t>(start),
                                           static_cast<std::size_t>(i - start)));

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
        return true;
    }

    bool Scan::consume_operator()
    {
        const char c = peek();
        if (!std::ispunct(static_cast<unsigned char>(c)))
            return false;

        const int start = i;
        ++i;
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
        return true;
    }
}
