#pragma once
// P3.6 — pure viewport-scroll geometry: the normal-mode scroll commands
// (C-f/C-b/C-d/C-u, H/M/L, zz/zt/zb) and the follow-the-cursor rule. No
// ncurses, no editing: the ui layer only renders what this computes.
//
// All line indices are 0-based (`top` = first buffer line on screen).
// Ground truth: tests/diff/motion_p3f.fxt, recorded from
// `nvim --headless -u NONE` with a 22-row window (see the gen script).
#include <cstdint>

namespace noni {

// Geometry of one window view. `rows` is the height available for buffer
// text (statusline/prompt rows already removed). `line_count` is always >= 1
// (an empty buffer still has one line).
struct ViewGeometry {
    std::uint32_t line_count = 1;
    int rows = 1;
    int scrolloff = 0; // vim 'scrolloff' rows kept above/below the cursor
};

enum class ScrollId : std::uint8_t {
    PageDown,   // C-f: [count] pages down, cursor to the top of the new page
    PageUp,     // C-b: [count] pages up, cursor to the bottom of the new page
    HalfDown,   // C-d: [count] lines down (no count: half a page)
    HalfUp,     // C-u: [count] lines up (no count: half a page)
    LineTop,    // H: [count] lines below the top of the view
    LineMiddle, // M: middle line of the view
    LineBottom, // L: [count] lines above the bottom of the view
    Center,     // zz: [count] line in the middle (no count: the cursor line)
    Top,        // zt: [count] line at the top (no count: the cursor line)
    Bottom,     // zb: [count] line at the bottom (no count: the cursor line)
};

// One scroll command: the window's current (cursor line, top) plus the typed
// [count]. `count` means a repeat (pages for C-f/C-b), an exact line count
// (C-d/C-u) or a 1-BASED line number (zz/zt/zb); `counted` distinguishes "no
// count typed" from a typed 1 (vim: `1zt` targets line 1 while plain `zt`
// targets the cursor line, and plain C-d scrolls half a page).
struct ScrollRequest {
    ScrollId id = ScrollId::HalfDown;
    std::uint32_t cursor_line = 0;
    std::uint32_t top = 0;
    std::uint32_t count = 1;
    bool counted = false;
};

struct ScrollResult {
    std::uint32_t cursor_line = 0;
    std::uint32_t top = 0;
};

// Applies one scroll command. The cursor line always stays inside the view;
// the column is NOT touched here (the caller carries/clamps it, vertical
// semantics).
[[nodiscard]] ScrollResult apply_scroll(const ViewGeometry& g,
                                        const ScrollRequest& r);

// Rows of actual buffer text visible from `top` (fewer than `rows` when the
// view runs past EOF and draws '~' lanes).
[[nodiscard]] std::uint32_t visible_rows(const ViewGeometry& g,
                                         std::uint32_t top);

// After ANY cursor move: the top line that keeps the cursor visible, honouring
// `scrolloff`; returns `top` unchanged when the cursor already is in view.
[[nodiscard]] std::uint32_t follow_top(const ViewGeometry& g,
                                       std::uint32_t cursor_line,
                                       std::uint32_t top);

} // namespace noni
