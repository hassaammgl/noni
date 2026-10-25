#pragma once
// P2.x — ui umbrella: screen init/teardown, key decode, view + statusline
// layout (real module replaced the P0.2 stub view-by-view from P2.1).
#include "input.hpp"
#include "screen.hpp"
#include "statusline.hpp"
#include "view.hpp"

namespace noni::ui {
bool screen_ready();
} // namespace noni::ui
