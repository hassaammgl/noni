#include "syntax_lex.hpp"

namespace syntax_lex
{
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
}
