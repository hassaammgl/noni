#pragma once
// P2.7 + P3 — motion framework: vim-exact clamping primitive moves +
// word/find motions. Pure core: no ncurses, no editing writes (motions only
// position the cursor). Ground truth: tests/diff/motion_p2.fxt, motion_p3.fxt
// (w/W), motion_p3b.fxt (b/B/e/E), motion_p3c.fxt (f/F/t/T/;/,) — recorded
// from nvim --headless -u NONE; regenerate via the gen_motion*.vim scripts.
#include <cstdint>
#include <vector>

#include "core/pos.hpp"
#include "core/text.hpp"

namespace noni {

enum class MotionId : std::uint8_t {
    H,      // left; count steps, stops at col 0
    L,      // right; stops at the LAST CHAR (never past it)
    J,      // down; col clamps to the shorter line (no desired-col memory)
    K,      // up; same clamp
    Zero,   // col 0 (count is ignored, vim-style)
    Dollar, // line end (last char); with count N: end of line (N-1) below,
            // clamped to the last line — but it REJECTS when the start is
            // already the last line (oracle probes n2..n8)
    Word,   // 'w'  start of next word (word/WORD class runs, P3.1)
    WordUpper, // 'W' start of next WORD (blank-vs-nonblank runs, P3.1)
    WordBack,      // 'b' start of previous word (P3.2)
    WordBackUpper, // 'B' start of previous WORD (P3.2)
    WordEnd,       // 'e' end of next word (P3.2)
    WordEndUpper,  // 'E' end of next WORD (P3.2)
    FindFwd,  // 'f': count-th occurrence of Motion::arg RIGHT of the col
              // (current line only; FAILS — no perceived motion) (P3.3)
    FindBack, // 'F': count-th occurrence LEFT of the col (P3.3)
    ToFwd,    // 't': lands one char BEFORE FindFwd's char (P3.3)
    ToBack,   // 'T': lands one char AFTER FindBack's char (P3.3)
    FirstNonblank, // '^': first non-blank of the CURRENT line (P3.4).
                   // Count ignored; blank-only line -> LAST char; empty -> 0.
    GotoFirst, // 'gg': line [count] from the top, col CARRIED like j/k (P3.4)
    GotoLast,  // 'G': line [count] or the last one, col carried (P3.4)
    ParaFwd,   // '}'  next paragraph boundary: first EMPTY line below (P3.5)
    ParaBack,  // '{'  same upward (P3.5)
};

// What a motion sees: the line vector + tab expansion width (w/b/e etc in
// P3 need display widths; P2.7 moves ignore it).
struct MotionContext {
    const std::vector<std::string>* lines = nullptr;
    int tabwidth = 8;
};

struct Motion {
    MotionId id = MotionId::H;
    char32_t arg = 0; // find targets: the searched {char}

    // Clamped destination; most motions never fail (line-end clamping by
    // design). FIND MOTIONS DO: no occurrence in the line -> error (cursor
    // unchanged; the reducer surfaces the message, P3.3).
    // Normal-mode cursor never sits PAST the last char (vim rule): col
    // targets are [0, len-1], with len==0 -> col 0.
    //
    // `desired` is the caller's curswant (see WindowState::desired_col):
    // only J/K read it (col = clamp(desired) on the target line) and leave
    // it untouched; the reducer resets it on every other cursor move.
    [[nodiscard]] eresult<Pos> apply(const MotionContext& ctx, Pos from,
                                     std::uint32_t count,
                                     std::uint32_t desired) const;
};

} // namespace noni
