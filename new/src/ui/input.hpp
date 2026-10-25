#pragma once
// P2.1/P2.2 — key DECODER + reader. The key model itself lives in
// core/editor_state.hpp (the app layer dispatches on it); ui decodes terminal
// bytes into those plain Keys. The DECODER is pure (no ncurses calls): the
// live KeyReader feeds it one `get_wch` result at a time, tests feed a
// recorded stream. No editing logic here (ARCHITECTURE).
//
// TTY facts encoded below (documented edges):
// - Ctrl+J is 0x0A, indistinguishable from Enter in a terminal byte stream —
//   both decode to Enter (13/CRLF decodes to Enter as well).
// - Ctrl+H (0x08) decodes to CtrlChar 'h' (127/KEY_BACKSPACE are Backspace).
// - Alt is an ESC PREFIX: Alt+a arrives as Esc then 'a' (two keys) until P6
//   gives mods meaning.
// - Bracketed paste: registered via define_key() so ncurses hands us
//   dedicated key codes; between the markers everything is Char DATA
//   (incl. \r/\n — dispatch turns Enter into a plain split while pasting).
// - TODO(P13): stdin EOF is a known v2 limitation — ncurses synthesizes one
//   EOT (0x04) and then hangs internally instead of reporting EOF. The app
//   exits on user quit (`:q`, P6) only; scripted feeders must end with a
//   normal-mode `q`. The P13 poll loop owns the terminal fd and fixes this.
#include <cstdint>
#include <functional>
#include <ncurses.h>
#include <string>
#include <utility>
#include <vector>

#include "core/editor_state.hpp"
#include "core/pos.hpp"

namespace noni::ui {

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
