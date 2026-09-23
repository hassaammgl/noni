#include "keybindings_ext.hpp"

#include <ncurses.h>
#include <term.h>
#ifdef lines
#undef lines
#endif
#ifdef columns
#undef columns
#endif

#include <array>

namespace keybindings_ext
{
    struct ExtDef
    {
        const char *seq;
        const char *cap;
        bool ctrl;
        bool alt;
        int base;
        int code = 0;
    };

    // CSI 1;3 = Alt, 1;5 = Ctrl, 1;7 = Ctrl+Alt (xterm / tmux / kitty legacy).
    std::array<ExtDef, 32> g_ext{{
        {"\033[1;3A", "kUP3", false, true, KEY_UP},
        {"\033[1;3B", "kDN3", false, true, KEY_DOWN},
        {"\033[1;3C", "kRIT3", false, true, KEY_RIGHT},
        {"\033[1;3D", "kLFT3", false, true, KEY_LEFT},
        {"\033[1;5A", "kUP5", true, false, KEY_UP},
        {"\033[1;5B", "kDN5", true, false, KEY_DOWN},
        {"\033[1;5C", "kRIT5", true, false, KEY_RIGHT},
        {"\033[1;5D", "kLFT5", true, false, KEY_LEFT},
        {"\033[1;7A", "kUP7", true, true, KEY_UP},
        {"\033[1;7B", "kDN7", true, true, KEY_DOWN},
        {"\033[1;7C", "kRIT7", true, true, KEY_RIGHT},
        {"\033[1;7D", "kLFT7", true, true, KEY_LEFT},
        {"\033[1;3H", "kHOM3", false, true, KEY_HOME},
        {"\033[1;3F", "kEND3", false, true, KEY_END},
        {"\033[1;5H", "kHOM5", true, false, KEY_HOME},
        {"\033[1;5F", "kEND5", true, false, KEY_END},
        {"\033[5;3~", "kPRV3", false, true, KEY_PPAGE},
        {"\033[6;3~", "kNXT3", false, true, KEY_NPAGE},
        {"\033[5;5~", "kPRV5", true, false, KEY_PPAGE},
        {"\033[6;5~", "kNXT5", true, false, KEY_NPAGE},
        {"\033[3;3~", "kDC3", false, true, KEY_DC},
        {"\033[3;5~", "kDC5", true, false, KEY_DC},
        {"\033[2;3~", "kIC3", false, true, KEY_IC},
        {"\033[2;5~", "kIC5", true, false, KEY_IC},
        {"\033[1;3~", nullptr, false, true, KEY_HOME},
        {"\033[4;3~", nullptr, false, true, KEY_END},
        {"\033[1;5~", nullptr, true, false, KEY_HOME},
        {"\033[4;5~", nullptr, true, false, KEY_END},
        {"\033\033[A", nullptr, false, true, KEY_UP},
        {"\033\033[B", nullptr, false, true, KEY_DOWN},
        {"\033\033[C", nullptr, false, true, KEY_RIGHT},
        {"\033\033[D", nullptr, false, true, KEY_LEFT},
    }};

    void register_extended_keys()
    {
        use_extended_names(TRUE);
        int code = KEY_MAX - 1;
        for (auto &e : g_ext)
        {
            e.code = code--;
            if (e.seq && e.seq[0])
                define_key(e.seq, e.code);
            if (e.cap)
            {
                const char *s = tigetstr(e.cap);
                if (s != nullptr && s != reinterpret_cast<char *>(-1) && s[0])
                    define_key(s, e.code);
            }
        }
    }

    bool apply_extended(int raw, KeyToken &t)
    {
        for (const auto &e : g_ext)
        {
            if (e.code != 0 && e.code == raw)
            {
                t.ctrl = e.ctrl;
                t.alt = e.alt;
                t.code = e.base;
                return true;
            }
        }
        return false;
    }

    const char *seq_for(int raw)
    {
        for (const auto &e : g_ext)
        {
            if (e.code != 0 && e.code == raw)
                return e.seq;
        }
        return nullptr;
    }
}
