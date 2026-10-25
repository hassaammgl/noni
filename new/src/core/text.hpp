#pragma once
// P1.1 — text primitives: UTF-8 decode/encode, codepoint<->byte mapping in a
// single line, char classes for word motions, display width (tabs + wide cps).
// Pure: no ncurses, no app state, no exceptions (see AGENTS.md).
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "core/pos.hpp"

namespace noni {

// Vim-style char classes. `Word` = letters/digits/underscore (unicode letters
// are Word too), `Punct` = visible non-word glyphs, `Whitespace` = blanks.
enum class CharClass : std::uint8_t {
    Word,
    Punct,
    Whitespace,
};

// --- UTF-8 decode/encode ----------------------------------------------------

// Decodes one codepoint starting at `byte`. Returns `{cp, byte_len}` where
// `byte_len` is 1..4. Well-formed UTF-8 only ahead: overlong encodings,
// surrogates (U+D800..DFFF) and > U+10FFFF are errors. The caller decides how
// to treat malformed lines (the buffer path treats one bad byte as one
// pseudo-codepoint, preserving raw bytes).
[[nodiscard]] eresult<std::pair<char32_t, std::size_t>>
decode_utf8(std::string_view line, std::size_t byte);

// Encodes a scalar value (caller must not pass lone surrogates).
[[nodiscard]] std::string encode_utf8(char32_t cp);

// --- positions in one line (codepoint col <-> byte offset) -------------------
//
// All of these are LENIENT: a malformed byte counts as exactly one
// pseudo-codepoint (advancing one byte), and out-of-range positions clamp.
// This keeps raw-byte files addressable instead of corrupting them.

[[nodiscard]] std::uint32_t line_len_cp(std::string_view line);

// Codepoint index -> byte offset (clamped to [0, line.size()]).
[[nodiscard]] std::size_t cp_to_byte(std::string_view line, std::uint32_t col);

// Byte offset -> codepoint index (clamped; a byte in the MIDDLE of a
// multi-byte sequence maps to the codepoint that owns it).
[[nodiscard]] std::uint32_t byte_to_cp(std::string_view line, std::size_t byte);

// --- char classes ------------------------------------------------------------

[[nodiscard]] CharClass classify(char32_t cp);
[[nodiscard]] bool is_blank(char32_t cp);
// True when `a` and `b` belong to the same motion class (used by w/b/e: word
// chars group with word chars, punch with punct; Whitespace is never "same").
[[nodiscard]] bool same_word_class(char32_t a, char32_t b);

// --- display width -----------------------------------------------------------

// Columns a codepoint occupies at display column `disp_col`. Tab advances to
// the next multiple of `tabwidth` (minimum 1); East-Asian wide ranges take 2
// columns; everything else takes 1.
[[nodiscard]] int cp_display_width(char32_t cp, int tabwidth, int disp_col);

// Default tab expansion until the config module lands (P18). Owned by core
// so app (motions) and ui (rendering) always agree.
inline constexpr int k_default_tabwidth = 4;

} // namespace noni
