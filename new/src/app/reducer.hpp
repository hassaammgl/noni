#pragma once
// P2.2 — THE edit path (ARCHITECTURE §4): every state change from input flows
// through reducer(). app owns mode-aware key -> Command translation; ui only
// decodes bytes into core Keys.
#include "core/editor_state.hpp"

namespace noni::app {

// Mode-aware translation (Normal/Insert maps differ; invalid/ignored keys
// yield Kind::None). Pure over state + Key.
[[nodiscard]] Command command_for_key(const EditorState& st, const Key& k);

// Applies one command. No-op-safe on invalid BufferId; always clamps the
// active window cursor back into the buffer (v0 clamp semantics; vim-exact
// motion rules are P2.7 via core/motion.hpp).
void reducer(EditorState& st, const Command& cmd);

} // namespace noni::app
