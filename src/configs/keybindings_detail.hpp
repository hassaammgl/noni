#pragma once
#include <configs/keybindings.hpp>
#include <commands/command_registry.hpp>
#include <utils/logger.hpp>
#include <utils/str.hpp>

#include <format>

#include <ncurses.h>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace keybindings_detail
{
    inline std::string lower(std::string s)
    {
        for (char &c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    inline KeyToken parse_single(std::string part)
    {
        part = StrUtils::trim(part);
        KeyToken t;

        while (true)
        {
            const auto plus = part.find('+');
            if (plus == std::string::npos)
                break;

            std::string mod = lower(part.substr(0, plus));
            part = part.substr(plus + 1);
            if (mod == "ctrl" || mod == "control" || mod == "c")
                t.ctrl = true;
            else if (mod == "alt" || mod == "option" || mod == "a")
                t.alt = true;
            else if (mod == "shift" || mod == "s")
                t.shift = true;
        }

        part = StrUtils::trim(part);
        const std::string name = lower(part);

        if (name == "escape" || name == "esc")
            t.code = 27;
        else if (name == "tab")
            t.code = '\t';
        else if (name == "enter" || name == "return")
            t.code = '\n';
        else if (name == "space")
            t.code = ' ';
        else if (name == "backspace" || name == "bs")
            t.code = 127;
        else if (name == "up")
            t.code = KEY_UP;
        else if (name == "down")
            t.code = KEY_DOWN;
        else if (name == "left")
            t.code = KEY_LEFT;
        else if (name == "right")
            t.code = KEY_RIGHT;
        else if (name == "pageup" || name == "pgup")
            t.code = KEY_PPAGE;
        else if (name == "pagedown" || name == "pgdn" || name == "pgdown")
            t.code = KEY_NPAGE;
        else if (name == "home")
            t.code = KEY_HOME;
        else if (name == "end")
            t.code = KEY_END;
        else if (name == "delete" || name == "del")
            t.code = KEY_DC;
        else if (name == "insert" || name == "ins")
            t.code = KEY_IC;
        else if (name == "slash")
            t.code = '/';
        else if (name == "period" || name == "dot")
            t.code = '.';
        else if (name == "comma")
            t.code = ',';
        else if (name == "semicolon")
            t.code = ';';
        else if (name == "colon")
            t.code = ':';
        else if (name == "quote" || name == "doublequote")
            t.code = '"';
        else if (name == "apostrophe" || name == "squote")
            t.code = '\'';
        else if (name == "question")
            t.code = '?';
        else if (name == "bang" || name == "exclaim")
            t.code = '!';
        else if (name == "`" || name == "backtick" || name == "grave")
            t.code = '`';
        else if (name == "\\" || name == "backslash")
            t.code = '\\';
        else if (name == "[" || name == "leftbracket")
            t.code = '[';
        else if (name == "]" || name == "rightbracket")
            t.code = ']';
        else if (name == "-" || name == "minus")
            t.code = '-';
        else if (name == "=" || name == "equals" || name == "plus")
            t.code = '=';
        else if (name == "_" || name == "underscore")
            t.code = '_';
        else if (name.size() >= 2 && name[0] == 'f')
        {
            int n = 0;
            bool ok = true;
            for (std::size_t i = 1; i < name.size(); ++i)
            {
                if (name[i] < '0' || name[i] > '9')
                {
                    ok = false;
                    break;
                }
                n = n * 10 + (name[i] - '0');
            }
            if (ok && n >= 1 && n <= 12)
                t.code = KEY_F(n);
        }
        else if (name.size() == 1)
        {
            char c = name[0];
            if (t.shift && c >= 'a' && c <= 'z')
                c = static_cast<char>(c - 'a' + 'A');
            t.code = static_cast<unsigned char>(c);
            if (c >= 'A' && c <= 'Z')
                t.shift = true;
        }
        else if (part.size() == 1)
        {
            t.code = static_cast<unsigned char>(part[0]);
            if (part[0] >= 'A' && part[0] <= 'Z')
                t.shift = true;
        }

        return t;
    }
}
