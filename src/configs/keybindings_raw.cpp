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
#include "keybindings_ext.hpp"

using namespace keybindings_detail;

KeyToken KeybindingEngine::from_raw(int raw_key)
{
    KeyToken t;
    if (keybindings_ext::apply_extended(raw_key, t))
        return t;

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
    // nonl(): Enter is KEY_ENTER or CR (13). LF (10) is Ctrl+J; BS-byte 8 is Ctrl+H.
    if (raw_key == KEY_ENTER || raw_key == '\r')
    {
        t.code = '\n';
        return t;
    }
    if (raw_key == KEY_BACKSPACE || raw_key == 127)
    {
        t.code = 127;
        return t;
    }
    if (raw_key == 8)
    {
        t.ctrl = true;
        t.code = 'h';
        return t;
    }
    if (raw_key == '\n')
    {
        t.ctrl = true;
        t.code = 'j';
        return t;
    }

    if (raw_key >= KEY_F(1) && raw_key <= KEY_F(12))
    {
        t.code = raw_key;
        return t;
    }
    if (raw_key >= KEY_F(13) && raw_key <= KEY_F(24))
    {
        t.shift = true;
        t.code = KEY_F(raw_key - KEY_F(13) + 1);
        return t;
    }

    if (raw_key >= 1 && raw_key <= 26)
    {
        t.ctrl = true;
        t.code = 'a' + (raw_key - 1);
        return t;
    }
    if (raw_key == 28)
    {
        t.ctrl = true;
        t.code = '\\';
        return t;
    }
    if (raw_key == 29)
    {
        t.ctrl = true;
        t.code = ']';
        return t;
    }
    if (raw_key == 31)
    {
        t.ctrl = true;
        t.code = '/';
        return t;
    }
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
