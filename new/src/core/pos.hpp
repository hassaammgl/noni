#pragma once
// P1.1 — fundamental types (ARCHITECTURE §3). Header-only; pure.
#include <compare>
#include <cstdint>
#include <expected>
#include <string>

namespace noni {

using BufferId = std::uint32_t;
using WindowId = std::uint32_t;

inline constexpr BufferId k_no_buffer = 0; // 0 is always invalid
inline constexpr WindowId k_no_window = 0;

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
