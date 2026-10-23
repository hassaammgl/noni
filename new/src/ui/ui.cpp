#include "ui/ui.hpp"

// P0.2 layer check: only ui/ sees ncurses headers (CMake scoping above).
#include <curses.h>

namespace noni::ui {

bool screen_ready() { return ::stdscr == nullptr; }

} // namespace noni::ui
