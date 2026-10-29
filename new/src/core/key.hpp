#pragma once
// P4.1 — typed keys, moved out of editor_state.hpp so that core/operator.hpp
// can consume them without pulling the whole editor state in (no include
// cycle: editor_state.hpp includes THIS plus operator.hpp). The ui layer
// DECODES terminal bytes into these plain Keys; nothing here touches ncurses.
#include <cstdint>

namespace noni {

enum class KeyKind : std::uint8_t {
    None,   // decoder-internal step
    Char,   // any codepoint input (cp); bracketed-paste data ships as Char
    Esc,
    Enter,
    Backspace,
    Tab,
    CtrlChar, // cp holds the letter ('a'..'z'); Ctrl+J == Enter (documented)
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    Home,
    End,
    PageUp,
    PageDown,
    Delete,
    Resize,
    PasteBegin,
    PasteEnd,
    Unknown,
};

struct Key {
    KeyKind kind = KeyKind::None;
    char32_t cp = 0; // Char: the codepoint; CtrlChar: the letter
    friend bool operator==(const Key&, const Key&) = default;
};

} // namespace noni
