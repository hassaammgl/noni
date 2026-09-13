#pragma once

#include <terminal/terminal_cell.hpp>

#include <deque>
#include <string>
#include <vector>

// Grid + scrollback for one terminal viewport. No ncurses, no PTY.
class TerminalScreen
{
public:
    void resize(int rows, int cols);
    void reset();
    void clear();

    int rows() const { return rows_; }
    int cols() const { return cols_; }
    int cursor_row() const { return cursor_row_; }
    int cursor_col() const { return cursor_col_; }

    const TerminalAttrs &pen() const { return pen_; }
    TerminalAttrs &pen() { return pen_; }

    void set_scrollback_limit(int n);
    int scrollback_limit() const { return scrollback_limit_; }
    int scrollback_lines() const { return static_cast<int>(scrollback_.size()); }

    // Live screen cell at (row,col). Out of range → empty cell.
    const TerminalCell &at(int row, int col) const;
    // Line from scrollback (0 = oldest) or live (offset by scrollback size).
    // view_row 0 is top of composed history+screen for scroll rendering helpers.
    void snapshot_view_line(int history_row, std::vector<TerminalCell> &out) const;
    int total_history_rows() const;

    // Cursor / writing
    void set_cursor(int row, int col);
    void move_cursor(int drow, int dcol);
    void save_cursor();
    void restore_cursor();

    void put_codepoint(char32_t cp);
    void newline();
    void carriage_return();
    void backspace();
    void tab();

    void erase_in_display(int mode); // 0 below, 1 above, 2 all, 3 all+scrollback
    void erase_in_line(int mode);    // 0 to end, 1 to start, 2 all

    void set_scroll_region(int top, int bottom); // 0-based inclusive
    void reset_scroll_region();
    void index();      // move down / scroll up
    void reverse_index();

    void set_wraparound(bool on) { wraparound_ = on; }
    bool wraparound() const { return wraparound_; }

    void set_title(std::string title) { title_ = std::move(title); }
    const std::string &title() const { return title_; }

private:
    using Line = std::vector<TerminalCell>;

    Line make_blank_line() const;
    void clamp_cursor();
    void scroll_up(int n);
    void scroll_down(int n);
    void erase_cells(int row, int c0, int c1);
    void put_glyph(char32_t cp, int width);
    void push_scrollback(Line line);

    int rows_ = 0;
    int cols_ = 0;
    std::vector<Line> grid_;
    std::deque<Line> scrollback_;
    int scrollback_limit_ = 5000;

    int cursor_row_ = 0;
    int cursor_col_ = 0;
    int saved_row_ = 0;
    int saved_col_ = 0;
    TerminalAttrs saved_pen_{};

    int scroll_top_ = 0;
    int scroll_bottom_ = 0; // inclusive; = rows_-1 when full
    bool wraparound_ = true;
    bool wrap_pending_ = false;

    TerminalAttrs pen_{};
    std::string title_;
    TerminalCell empty_{};
};
