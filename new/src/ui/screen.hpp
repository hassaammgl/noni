#pragma once
// P2.1 — ncursesw screen init/teardown (RAII owner of the terminal session).
// Layout splitting arrives in P2.4; until then the Screen owns ONE full-screen
// window. ui/ only; nothing below may include ncurses (layer lint P0.2).
//
// Init contract: noecho + cbreak, ESCDELAY 25ms, keypad on (CSI/KEY_* resolve),
// meta on, colors initialized with default-background pairs when supported.
// KEY_RESIZE recreates the window (App::on_resize wiring lands in P2.5).
#include <memory>

#include <ncurses.h>

#include "core/pos.hpp"
#include "ui/input.hpp"

namespace noni::ui {

class Screen {
public:
    [[nodiscard]] static eresult<Screen> open();

    ~Screen();
    Screen(Screen&& other) noexcept;
    Screen& operator=(Screen&&) = delete;
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;

    // TEMP (P2.1..P2.4): bottom-row debug line showing the last key decoded.
    void debug_key(const Key& k);

    void on_resize(); // re-creates the window for the new LINES/COLS

    [[nodiscard]] int rows() const { return rows_; }
    [[nodiscard]] int cols() const { return cols_; }
    [[nodiscard]] WINDOW* win() const { return win_.get(); }
    void redraw() { wrefresh(win_.get()); }

private:
    Screen() = default;

    struct WindowDeleter {
        void operator()(WINDOW* w) const;
    };

    std::unique_ptr<WINDOW, WindowDeleter> win_;
    int rows_ = 0;
    int cols_ = 0;
    bool colors_ = false;
};

} // namespace noni::ui
