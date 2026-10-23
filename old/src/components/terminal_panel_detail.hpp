#pragma once
#include <components/terminal_panel.hpp>
#include <ui/theme.hpp>

#include <algorithm>
#include <ncurses.h>
#include <vector>

namespace terminal_panel_detail
{
    inline void encode_utf8(char32_t cp, char out[8], int &len)
    {
        len = 0;
        if (cp <= 0x7F)
        {
            out[len++] = static_cast<char>(cp);
        }
        else if (cp <= 0x7FF)
        {
            out[len++] = static_cast<char>(0xC0 | (cp >> 6));
            out[len++] = static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp <= 0xFFFF)
        {
            out[len++] = static_cast<char>(0xE0 | (cp >> 12));
            out[len++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out[len++] = static_cast<char>(0x80 | (cp & 0x3F));
        }
        else
        {
            out[len++] = static_cast<char>(0xF0 | (cp >> 18));
            out[len++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out[len++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out[len++] = static_cast<char>(0x80 | (cp & 0x3F));
        }
        out[len] = 0;
    }

    // xterm: F1–F4 SS3, F5–F12 CSI. Returns C string + writes byte count.
    inline const char *xterm_f_seq(int key, int &n)
    {
        switch (key)
        {
        case KEY_F(1):
            n = 3;
            return "\033OP";
        case KEY_F(2):
            n = 3;
            return "\033OQ";
        case KEY_F(3):
            n = 3;
            return "\033OR";
        case KEY_F(4):
            n = 3;
            return "\033OS";
        case KEY_F(5):
            n = 5;
            return "\033[15~";
        case KEY_F(6):
            n = 5;
            return "\033[17~";
        case KEY_F(7):
            n = 5;
            return "\033[18~";
        case KEY_F(8):
            n = 5;
            return "\033[19~";
        case KEY_F(9):
            n = 5;
            return "\033[20~";
        case KEY_F(10):
            n = 5;
            return "\033[21~";
        case KEY_F(11):
            n = 5;
            return "\033[23~";
        case KEY_F(12):
            n = 5;
            return "\033[24~";
        default:
            n = 0;
            return nullptr;
        }
    }
}
