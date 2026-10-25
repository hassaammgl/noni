#include "ui/ui.hpp"

// P2.1 layer check: only ui/ sees ncurses headers (CMake scoping above).
#include <curses.h>

namespace noni::ui {

// "Ready" = the real Screen type is available and linked (P2.1). The curses
// include above is the layer-lint tripwire (compiling here means ui/ only).
bool screen_ready() { return true; }

} // namespace noni::ui
