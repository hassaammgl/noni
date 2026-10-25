#pragma once
// P0.2 stub growing real: App skeleton arrives P2.2 (EditorState + reducer),
// the loop P2.5. P2.1 edge only needs the resize hook defined.
namespace noni::app {

bool app_ready();

// P2.1 edge skeleton — KEY_RESIZE will call this from the main loop (P2.5);
// the real body arrives with App (P2.2/P2.5).
inline void on_resize() {}

} // namespace noni::app
