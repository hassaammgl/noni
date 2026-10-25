#pragma once
// P2.2 — editor domain state + command model. Neither includes ncurses nor
// anything ui/: the ui layer DECODES terminal bytes into these plain Keys and
// the app layer reduces Commands against this state (ARCHITECTURE §4: one
// reducer is the single edit path).
#include <cstdint>
#include <string>
#include <vector>

#include "core/buffer.hpp"
#include "core/motion.hpp"
#include "core/pos.hpp"

namespace noni {

// --- typed keys (P2.1 model, moved to core in P2.2) ---------------------------

enum class KeyKind : std::uint8_t {
    None,   // decoder-internal step
    Char,   // any codepoint input (cp); bracketed-paste data ships as Char
    Esc,
    Enter,
    Backspace,
    Tab,
    CtrlChar, // cp holds the letter ('a'..'z'); Ctrl+J == Enter (documented)
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    Home,
    End,
    PageUp,
    PageDown,
    Delete,
    Resize,
    PasteBegin,
    PasteEnd,
    Unknown,
};

struct Key {
    KeyKind kind = KeyKind::None;
    char32_t cp = 0; // Char: the codepoint; CtrlChar: the letter
    friend bool operator==(const Key&, const Key&) = default;
};

// --- modes/window/buffers ------------------------------------------------------

enum class Mode : std::uint8_t {
    Normal,
    Insert, // cmdline/config modes arrive with P6
};

struct WindowState {
    BufferId buffer = k_no_buffer;
    Pos cursor{};      // (line, cp col) — ALWAYS clamped to the buffer
    std::uint32_t top_line = 0;    // viewport scroll (P2.3)
    std::uint32_t left_col = 0;    // horizontal scroll (P2.3)
    std::uint32_t desired_col = 0; // vim curswant: the col j/k aim at across
                                   // shorter lines; any non-vertical cursor
                                   // move resets it to the new cursor col
};

// --- command model (mode-agnostic; app/reducer translates keys) ---------------

struct Command {
    enum class Kind : std::uint8_t {
        None,          // key produced no command in the current mode
        Motion,        // `motion` payload; count applied by the reducer
        CountDigit,    // normal-mode digit extension of `count_prefix`
        InsertChar,    // insert mode only
        InsertNewline, // Enter in insert: split + copy previous line's indent
        SplitLine,     // '\n' inside a bracketed paste: split, NO indent copy
        PasteBegin,    // st.pasting = true (bracketed-paste session)
        PasteEnd,      // st.pasting = false
        EraseChar,     // insert-mode backspace (left of the cursor; joins)
        DeleteChar,    // normal-mode deletion at the cursor ('x', count>1 ok)
        EnterInsert,   // `at_eol` selects 'i' vs 'A' (append at line end)
        LeaveInsert,
        Undo,
        Redo,
    };
    Kind kind = Kind::None;
    MotionId motion = MotionId::H; // Motion payload
    char32_t cp = 0;               // InsertChar / CountDigit payload
    std::uint32_t count = 1;       // reserved for P3 command syntax
    bool at_eol = false;           // EnterInsert flavor ('A' appends EOL)
};

class EditorState {
public:
    // Fresh state: one empty buffer + one window on it, normal mode.
    EditorState();

    // Load a file as a NEW buffer and open it in the active window.
    [[nodiscard]] eresult<BufferId> open_file(const std::filesystem::path& p);

    // --- accessors (no exceptions; invalid ids resolve to nullptr) ----------
    [[nodiscard]] Buffer* buffer(BufferId id);
    [[nodiscard]] const Buffer* buffer(BufferId id) const;
    // Buffer assignment returns eresult<BufferId>:
    [[nodiscard]] BufferId buffer_id(const Buffer& b) const;

    [[nodiscard]] WindowState& win() {
        if (active_window_ >= windows.size()) {
            active_window_ = 0;
        }
        return windows[active_window_];
    }
    [[nodiscard]] const WindowState& win() const {
        return windows[active_window_ < windows.size() ? active_window_ : 0];
    }

    // Marks the render needed (P2.5 draws dirty state only).
    void mark_dirty() { needs_render_ = true; }
    [[nodiscard]] bool needs_render() const { return needs_render_; }
    void clear_render() { needs_render_ = false; }

    // --- public state surface (reducer owns mutations) ------------------------
    std::vector<Buffer> buffers;
    std::vector<WindowState> windows;
    Mode mode = Mode::Normal;
    bool pasting = false; // inside a bracketed paste ('\n' inserts as data)
    // Pending count prefix ('5j': count_prefix 5 until the next command).
    // 0 = none pending; a command consumes+resets it. '0' only STARTS count
    // after a nonzero digit (else it is the col-0 motion — oracle-pinned).
    std::uint32_t count_prefix = 0;
    std::string msg;      // statusline message (P2.4 renders)
    std::string cmdline;  // prompt row reservation (P6 fills it)

private:
    std::size_t active_window_ = 0;
    bool needs_render_ = true;
};

} // namespace noni
