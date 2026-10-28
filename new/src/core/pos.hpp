#pragma once
// P1.1 — fundamental types (ARCHITECTURE §3). Header-only; pure.
#include <compare>
#include <cstdint>
#include <expected>
#include <string>

namespace noni {

using BufferId = std::uint32_t;
using WindowId = std::uint32_t;

// Sentinels (P2.2): buffer/window ids are storage indexes starting at 0, so
// "none" is max-value, not 0. (P0 stub said 0 is invalid; EditorState's first
// buffer is id 0 — the sentinel moved with the indexes.)
inline constexpr BufferId k_no_buffer = 0xFFFFFFFF;
inline constexpr WindowId k_no_window = 0xFFFFFFFF;

// vim's MAXCOL: the "as far right as possible" curswant that '$' sets (P3.8).
// Vertical moves clamp it into whatever line they land on, so '$' followed by
// 'j' goes to the END of the next line, not to the previous line's last
// column. Everything else that moves the cursor stores a real column.
inline constexpr std::uint32_t k_max_col = 0x7FFFFFFFu;

// A codepoint position in a buffer: `(line, col)` where `col` is the
// CODEPOINT index inside that line (0 = first cp). Never a byte offset.
struct Pos {
    std::uint32_t line = 0;
    std::uint32_t col = 0;
    auto operator<=>(const Pos&) const = default;
};

// Inclusive `start`, exclusive `end` (codepoint positions). Cross-line ranges
// span full lines between the start and end lines.
struct TextRange {
    Pos start;
    Pos end;
    auto operator<=>(const TextRange&) const = default;
};

// Error type for every eresult in the project. No exceptions anywhere.
struct Error {
    std::string msg;
};

template <class T>
using eresult = std::expected<T, Error>;

// Sugar for the void-flavored success value.
[[nodiscard]] inline std::expected<void, Error> ok() { return {}; }

} // namespace noni
