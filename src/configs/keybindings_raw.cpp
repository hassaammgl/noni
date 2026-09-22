#include <configs/keybindings.hpp>
#include <commands/command_registry.hpp>
#include <utils/logger.hpp>
#include <utils/str.hpp>

#include <format>

#include <ncurses.h>
#include <algorithm>
#include <cctype>
#include <sstream>

#include "keybindings_detail.hpp"

using namespace keybindings_detail;

KeyToken KeybindingEngine::from_raw(int raw_key)
{
    KeyToken t;

    if (raw_key == 27)
    {
        t.code = 27;
        t.codepoint = 27;
        return t;
    }
    if (raw_key == '\t')
    {
        t.code = '\t';
        return t;
    }
    if (raw_key == '\n' || raw_key == KEY_ENTER)
    {
        t.code = '\n';
        return t;
    }
    if (raw_key == KEY_BACKSPACE || raw_key == 127 || raw_key == 8)
    {
        t.code = 127;
        return t;
    }

    if (raw_key == KEY_F(2))
    {
        t.code = KEY_F(2);
        return t;
    }
    if (raw_key == KEY_F(3))
    {
        t.code = KEY_F(3);
        return t;
    }
    if (raw_key == KEY_F(4))
    {
        t.code = KEY_F(4);
        return t;
    }
    if (raw_key == KEY_F(5))
    {
        t.code = KEY_F(5);
        return t;
    }
    if (raw_key == KEY_F(6))
    {
        t.code = KEY_F(6);
        return t;
    }
    if (raw_key == KEY_F(7))
    {
        t.code = KEY_F(7);
        return t;
    }
    if (raw_key == KEY_F(8))
    {
        t.code = KEY_F(8);
        return t;
    }
    if (raw_key == KEY_F(9))
    {
        t.code = KEY_F(9);
        return t;
    }
    if (raw_key == KEY_F(10))
    {
        t.code = KEY_F(10);
        return t;
    }
    if (raw_key == KEY_F(11))
    {
        t.code = KEY_F(11);
        return t;
    }
    if (raw_key == KEY_F(12))
    {
        t.code = KEY_F(12);
        return t;
    }

    if (raw_key >= 1 && raw_key <= 26)
    {
        t.ctrl = true;
        t.code = 'a' + (raw_key - 1);
        return t;
    }

    // Ctrl+\ (ASCII FS) — VS Code split binding.
    if (raw_key == 28)
    {
        t.ctrl = true;
        t.code = '\\';
        return t;
    }

    // Many terminals send NUL for Ctrl+Space (and Ctrl+@).
    if (raw_key == 0)
    {
        t.ctrl = true;
        t.code = ' ';
        return t;
    }

    if (raw_key >= 'A' && raw_key <= 'Z')
    {
        t.shift = true;
        t.code = raw_key;
        t.codepoint = static_cast<char32_t>(raw_key);
        return t;
    }

    t.code = raw_key;
    if (raw_key >= 32 && raw_key < 127)
        t.codepoint = static_cast<char32_t>(raw_key);
    return t;
}

