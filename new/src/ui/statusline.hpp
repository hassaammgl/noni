#pragma once
// P2.4 — bottom strips + single-window layout fill:
//   row 0          : tab-bar placeholder (colored at P8)
//   rows 1..C-3    : buffer view (P2.3)
//   row C-2        : statusline — mode flag, filename, [+], Ln,Col
//   row C-1        : message row (statusline.msg) / cmdline prompt (reserved)
// The TEMP key debug line (P2.1..P2.4) overlays the last row; P2.5 removes it.
#include "core/editor_state.hpp"
#include "ui/screen.hpp"
#include "ui/view.hpp"

namespace noni::ui {

// Full-frame render of the single-window layout (clears first).
void draw_layout(Screen& screen, EditorState& st);

} // namespace noni::ui
