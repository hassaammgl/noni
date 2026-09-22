#pragma once
#include <editor/ex_commands.hpp>

#include <ui/ui.hpp>
#include <utils/messages.hpp>
#include <utils/str.hpp>

#include <cctype>
#include <format>

namespace ex_commands_detail
{
    inline bool is_cmd_char(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    inline std::vector<std::string> split_args(std::string_view raw)
    {
        std::vector<std::string> args;
        std::string cur;
        bool in_quotes = false;

        for (std::size_t i = 0; i < raw.size(); ++i)
        {
            const char c = raw[i];
            if (c == '"')
            {
                in_quotes = !in_quotes;
                continue;
            }
            if (!in_quotes && std::isspace(static_cast<unsigned char>(c)))
            {
                if (!cur.empty())
                {
                    args.push_back(cur);
                    cur.clear();
                }
                continue;
            }
            cur.push_back(c);
        }

        if (!cur.empty())
            args.push_back(cur);

        return args;
    }
}
