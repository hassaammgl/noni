#pragma once
// P2.7 — motion framework: vim-exact clamping primitive moves. Pure core:
// no ncurses, no editing writes (motions only position the cursor).
// Ground truth: tests/diff/motion_p2.fxt (recorded from
// nvim --headless -u NONE; regenerate via tests/diff/gen_motion.vim).
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
};

// What a motion sees: the line vector + tab expansion width (w/b/e etc in
// P3 need display widths; P2.7 moves ignore it).
struct MotionContext {
    const std::vector<std::string>* lines = nullptr;
    int tabwidth = 8;
};

struct Motion {
    MotionId id = MotionId::H;

    // Clamped destination; never fails (line-end clamping by design).
    // Normal-mode cursor never sits PAST the last char (vim rule): col
    // targets are [0, len-1], with len==0 -> col 0.
    //
    // `desired` is the caller's curswant (see WindowState::desired_col):
    // only J/K read it (col = clamp(desired) on the target line) and leave
    // it untouched; the reducer resets it on every other cursor move.
    [[nodiscard]] Pos apply(const MotionContext& ctx, Pos from,
                            std::uint32_t count,
                            std::uint32_t desired) const;
};

} // namespace noni
