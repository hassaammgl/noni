#include <terminal/terminal_screen.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>

#include "terminal_screen_detail.hpp"

using namespace terminal_screen_detail;

void TerminalScreen::resize(int rows, int cols)
{
    rows = std::max(1, rows);
    cols = std::max(1, cols);
    if (rows == rows_ && cols == cols_ && !grid_.empty())
        return;

    const int old_rows = rows_;
    const int old_cols = cols_;
    std::vector<Line> old = std::move(grid_);

    rows_ = rows;
    cols_ = cols;
    grid_.assign(static_cast<std::size_t>(rows_), make_blank_line());

    const int copy_rows = std::min(rows_, old_rows);
    const int copy_cols = std::min(cols_, old_cols);
    for (int r = 0; r < copy_rows; ++r)
    {
        for (int c = 0; c < copy_cols; ++c)
            grid_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] =
                old[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
    }

    scroll_top_ = 0;
    scroll_bottom_ = rows_ - 1;
    clamp_cursor();
    wrap_pending_ = false;
}

void TerminalScreen::reset()
{
    pen_ = {};
    saved_pen_ = {};
    cursor_row_ = 0;
    cursor_col_ = 0;
    saved_row_ = 0;
    saved_col_ = 0;
    wraparound_ = true;
    wrap_pending_ = false;
    title_.clear();
    scrollback_.clear();
    if (rows_ > 0 && cols_ > 0)
    {
        grid_.assign(static_cast<std::size_t>(rows_), make_blank_line());
        reset_scroll_region();
    }
}

void TerminalScreen::clear()
{
    for (auto &line : grid_)
        std::fill(line.begin(), line.end(), TerminalCell{});
    cursor_row_ = 0;
    cursor_col_ = 0;
    wrap_pending_ = false;
}

void TerminalScreen::set_scrollback_limit(int n)
{
    scrollback_limit_ = std::max(0, n);
    while (static_cast<int>(scrollback_.size()) > scrollback_limit_)
        scrollback_.pop_front();
}

const TerminalCell &TerminalScreen::at(int row, int col) const
{
    if (row < 0 || col < 0 || row >= rows_ || col >= cols_)
        return empty_;
    return grid_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

int TerminalScreen::total_history_rows() const
{
    return static_cast<int>(scrollback_.size()) + rows_;
}

void TerminalScreen::snapshot_view_line(int history_row, std::vector<TerminalCell> &out) const
{
    out.clear();
    if (history_row < 0)
        return;
    const int sb = static_cast<int>(scrollback_.size());
    if (history_row < sb)
    {
        out = scrollback_[static_cast<std::size_t>(history_row)];
        if (static_cast<int>(out.size()) < cols_)
            out.resize(static_cast<std::size_t>(cols_), TerminalCell{});
        else if (static_cast<int>(out.size()) > cols_)
            out.resize(static_cast<std::size_t>(cols_));
        return;
    }
    const int live = history_row - sb;
    if (live < 0 || live >= rows_)
        return;
    out = grid_[static_cast<std::size_t>(live)];
}

void TerminalScreen::set_cursor(int row, int col)
{
    cursor_row_ = row;
    cursor_col_ = col;
    clamp_cursor();
    wrap_pending_ = false;
}

void TerminalScreen::move_cursor(int drow, int dcol)
{
    cursor_row_ += drow;
    cursor_col_ += dcol;
    clamp_cursor();
    wrap_pending_ = false;
}

void TerminalScreen::save_cursor()
{
    saved_row_ = cursor_row_;
    saved_col_ = cursor_col_;
    saved_pen_ = pen_;
}

void TerminalScreen::restore_cursor()
{
    cursor_row_ = saved_row_;
    cursor_col_ = saved_col_;
    pen_ = saved_pen_;
    clamp_cursor();
    wrap_pending_ = false;
}

void TerminalScreen::put_codepoint(char32_t cp)
{
    if (cp == U'\n')
    {
        newline();
        return;
    }
    if (cp == U'\r')
    {
        carriage_return();
        return;
    }
    if (cp == U'\b')
    {
        backspace();
        return;
    }
    if (cp == U'\t')
    {
        tab();
        return;
    }
    if (cp == U'\a')
        return;
    if (cp < 32)
        return;

    const int w = glyph_width(cp);
    put_glyph(cp, w);
}

void TerminalScreen::newline()
{
    carriage_return();
    index();
}

