#pragma once
// P2.1 — key model + decoder. The DECODER is pure (no ncurses calls): the live
// KeyReader feeds it one `get_wch` result at a time, tests feed a recorded
// stream. This keeps Esc/CSI/bracketed-paste resolution unit-testable without
// a real terminal (ARCHITECTURE: ui/ owns input, no editing logic here).
//
// TTY facts encoded below (documented edges):
// - Ctrl+J is 0x0A, indistinguishable from Enter in a terminal byte stream —
//   both decode to Enter (13/CRLF decodes to Enter as well).
// - Ctrl+H (0x08) decodes to CtrlChar 'h' (127/KEY_BACKSPACE are Backspace).
// - Alt is an ESC PREFIX: Alt+a arrives as Esc then 'a' (two keys) until P6
//   gives mods meaning.
// - Bracketed paste: ESC [ 200 ~ ... ESC [ 201 ~ — markers decode to
//   PasteBegin/PasteEnd, everything between passes through as Char DATA.
#include <cstdint>
#include <ncurses.h>
#include <string>
#include <utility>
#include <vector>

#include "core/pos.hpp"

namespace noni::ui {

enum class KeyKind : std::uint8_t {
    None, // decoder internal step, nothing to dispatch
    Char, // any codepoint input (cp); bracketed-paste content arrives as data
    Esc,
    Enter,
    Backspace,
    Tab,
    CtrlChar, // cp holds the letter ('a'..'z'); Ctrl+J is Enter, see above
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
    char32_t cp = 0; // valid for Char (the codepoint) / CtrlChar (the letter)
    friend bool operator==(const Key&, const Key&) = default;
};

// One result of ncurses get_wch: `is_key_code` true = ncurses KEY_* value
// (KEY_CODE_YES return), false = a decoded wide codepoint (OK return).
using Wch = std::pair<bool, wint_t>;

// Codes we REGISTER with define_key() in Screen::open() so bracketed paste
// resolves inside ncurses (below KEY_MAX, above assigned terminfo entries).
// Esc stays instantaneous; the byte-by-byte ESC[,fallback is for exotic
// terminals without it.
inline constexpr wint_t k_paste_begin_code = 0772;
inline constexpr wint_t k_paste_end_code = 0773;

// Pure decode step. Feeding codes in order yields keys; CSI assembly returns
// None until the final byte of the sequence arrives.
class KeyDecoder {
public:
    [[nodiscard]] Key feed(bool is_key_code, wint_t code);

private:
    // PasteData: content bytes pass through RAW until the PasteEnd code.
    enum class St : std::uint8_t { Idle, EscSeen, CsiDigits, PasteData } st_ =
        St::Idle;
    std::uint32_t csi_num_ = 0;
};

// Whole-stream convenience for fixtures/tests.
[[nodiscard]] std::vector<Key>
decode_stream(const std::vector<Wch>& codes);

// Stable debug rendering of one key ("temp debug line" P2.1..P2.4).
[[nodiscard]] std::string key_debug(const Key& k);

// Blocks on the window and returns the next decoded key. Errors when ncurses
// reports a hard input failure (ERR without a key code).
class KeyReader {
public:
    explicit KeyReader(WINDOW* w) : win_(w) {}
    [[nodiscard]] eresult<Key> next();

private:
    KeyDecoder dec_;
    WINDOW* win_;
};

} // namespace noni::ui
