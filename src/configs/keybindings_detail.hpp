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
        else if (name == "f2")
            t.code = KEY_F(2);
        else if (name == "f3")
            t.code = KEY_F(3);
        else if (name == "f4")
            t.code = KEY_F(4);
        else if (name == "f5")
            t.code = KEY_F(5);
        else if (name == "f6")
            t.code = KEY_F(6);
        else if (name == "f7")
            t.code = KEY_F(7);
        else if (name == "f8")
            t.code = KEY_F(8);
        else if (name == "f9")
            t.code = KEY_F(9);
        else if (name == "f10")
            t.code = KEY_F(10);
        else if (name == "f11")
            t.code = KEY_F(11);
        else if (name == "f12")
            t.code = KEY_F(12);
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
