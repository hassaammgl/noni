// P2.1 — see screen.hpp.
#include "ui/screen.hpp"

#include <cstdio>
#include <utility>

namespace noni::ui {

void Screen::WindowDeleter::operator()(WINDOW* w) const {
    if (w != nullptr) {
        delwin(w);
    }
}

eresult<Screen> Screen::open() {
    WINDOW* base = initscr();
    if (base == nullptr) {
        return std::unexpected(Error{"screen: initscr failed (no tty?)"});
    }
    Screen s;
    s.rows_ = LINES;
    s.cols_ = COLS;

    // Input regime per ROADMAP P2.1 (cbreak, not raw; Ctrl+C stays a signal).
    noecho();
    cbreak();
    if (set_escdelay(25) == ERR) {
        // Non-fatal: fall back to whatever ESCDELAY the environment had.
        std::fputs("screen: could not set ESCDELAY\n", stderr);
    }

    s.win_.reset(newwin(LINES, COLS, 0, 0));
    if (s.win_ == nullptr) {
        endwin();
        return std::unexpected(Error{"screen: newwin failed"});
    }
    keypad(s.win_.get(), TRUE); // CSI/KEY_* resolution + KEY_RESIZE
    meta(s.win_.get(), TRUE);

    if (has_colors()) {
        if (start_color() != ERR && use_default_colors() != ERR) {
            s.colors_ = true; // pairs get numbered by P2.3/P2.4 renderers
        }
    }

    // Bracketed paste resolves INSIDE ncurses (define_key): Esc stays
    // instantaneous and paste bytes arrive as dedicated key codes.
    define_key("\x1b[200~", static_cast<int>(k_paste_begin_code));
    define_key("\x1b[201~", static_cast<int>(k_paste_end_code));
    return s;
}

Screen::~Screen() {
    win_.reset();
    if (stdscr != nullptr) { // initscr succeeded earlier iff stdscr valid
        endwin();
    }
}

Screen::Screen(Screen&& other) noexcept
    : win_(std::move(other.win_)), rows_(other.rows_), cols_(other.cols_),
      colors_(other.colors_) {
    other.rows_ = 0;
    other.cols_ = 0;
}

void Screen::on_resize() {
    // ncurses already grew LINES/COLS (KEY_RESIZE path); rebuild our window.
    win_.reset();
    rows_ = LINES;
    cols_ = COLS;
    win_.reset(newwin(rows_, cols_, 0, 0));
    if (win_ == nullptr) {
        return; // hard failure: window stays empty, next resize can retry
    }
    keypad(win_.get(), TRUE);
    meta(win_.get(), TRUE);
    rows_ = rows_; // (no-op; LINES is authoritative)
}

void Screen::debug_key(const Key& k) {
    if (win_ == nullptr) {
        return;
    }
    const int y = rows_ - 1;
    mvwprintw(win_.get(), y, 0, "key: %-14s", key_debug(k).c_str());
    wclrtoeol(win_.get());
    wrefresh(win_.get());
}

} // namespace noni::ui
