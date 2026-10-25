#pragma once
// P2.1 — ui umbrella. The real screen module landed here (ncursesw
// init/teardown + key reader), not in P7 (revised UI-first ROADMAP).
#include "input.hpp"
#include "screen.hpp"

namespace noni::ui {
bool screen_ready();
} // namespace noni::ui
