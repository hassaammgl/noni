#include "syntax_lex.hpp"

namespace syntax_lex
{
    void Scan::scan_tokens()
    {
        kw = keywords_for(lang);

        while (i < n)
        {
            const char c = peek();

            if (c == ' ' || c == '\t')
            {
                ++i;
                continue;
            }

            if (c == '#' && (lang == Language::Cpp || lang == Language::C) && i == 0)
            {
                push_token(out, 0, n, TokenKind::Preprocessor);
                return;
            }

            if (consume_comment())
            {
                if (line_comment_end)
                    break;
                continue;
            }
            if (consume_string())
                continue;
            if (consume_number())
                continue;
            if (consume_ident())
                continue;
            if (consume_operator())
                continue;

            ++i;
        }
    }

    std::vector<SyntaxToken> highlight_generic(
        std::string_view line,
        Language lang,
        HighlightState &state)
    {
        std::vector<SyntaxToken> out;
        Scan s{line, lang, state, out, 0, static_cast<int>(line.size())};
        if (s.resume_multiline())
            return out;
        s.scan_tokens();
        return out;
    }
}
