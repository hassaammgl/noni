#pragma once
// P2.5 — App: owns the EditorState and THE main loop (ARCHITECTURE §5).
// Inversion keeps layering: app/ cannot include ui/, so the loop talks through
// two small interfaces that main wires to the ncurses side:
//   KeySource  — blocking key input (ui::KeyReader in main)
//   Renderer   — view_rows() + full-frame render + resize hook
// P2.5 polls only stdin today; `wake_fd` is the reserved slot that job-pool
// threads will write into when EventQueue lands (P6/P14).
#include <filesystem>
#include <string>

#include "app/reducer.hpp"
#include "core/editor_state.hpp"

namespace noni::app {

class KeySource {
public:
    virtual ~KeySource() = default;
    KeySource() = default;
    KeySource(const KeySource&) = delete;
    KeySource& operator=(const KeySource&) = delete;
    [[nodiscard]] virtual eresult<Key> next() = 0;
};

class Renderer {
public:
    virtual ~Renderer() = default;
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    // Height of the buffer view area in rows (for follow/set scroll).
    [[nodiscard]] virtual int view_rows() const = 0;
    // Full-frame render + cursor placement per the passed state.
    virtual void render(const EditorState& st) = 0;
    // The screen itself resized: rebuild terminal-side structures.
    virtual void on_screen_resize() = 0;
};

class App {
public:
    // `path` may be empty: bare `noni` runs with one empty buffer.
    explicit App(KeySource& keys, Renderer& renderer, int wake_fd = -1)
        : keys_(keys), renderer_(renderer), wake_fd_(wake_fd) {}

    // Loads `path` before the loop (errors surface in the message row).
    void load_file(const std::filesystem::path& path);

    [[nodiscard]] int run(); // exit code; returns on quit / hard input error

    // EditorState access (tests drive the loop body inductively too).
    [[nodiscard]] EditorState& state() { return st_; }

    // TEMP quit hook (normal-mode `q`) until :q lands with P6.
    void request_quit() { quit_ = true; }
    [[nodiscard]] bool quit_requested() const { return quit_; }

    // KEY_RESIZE path: rebuild terminal structures + clamp state.
    void on_resize();

private:
    void render_if_dirty();

    EditorState st_;
    KeySource& keys_;
    Renderer& renderer_;
    int wake_fd_;   // skeleton: used by the event drain below (P6/P16)
    bool quit_ = false;
    bool render_pending_ = true;
};

bool app_ready();

} // namespace noni::app
