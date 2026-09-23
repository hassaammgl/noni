#pragma once
#include <configs/keybindings.hpp>

namespace keybindings_ext
{
    void register_extended_keys();
    bool apply_extended(int raw, KeyToken &t);
    const char *seq_for(int raw);
}
