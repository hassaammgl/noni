#include <terminal/terminal_screen.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>

#include "terminal_screen_detail.hpp"

using namespace terminal_screen_detail;

void TerminalScreen::carriage_return()
{
    cursor_col_ = 0;
    wrap_pending_ = false;
}

void TerminalScreen::backspace()
{
    if (wrap_pending_)
    {
        wrap_pending_ = false;
        return;
    }
    if (cursor_col_ > 0)
        --cursor_col_;
}

void TerminalScreen::tab()
{
    const int next = ((cursor_col_ / 8) + 1) * 8;
    cursor_col_ = std::min(cols_ - 1, next);
    wrap_pending_ = false;
}

void TerminalScreen::erase_in_display(int mode)
{
    if (mode == 2 || mode == 3)
    {
        for (auto &line : grid_)
            std::fill(line.begin(), line.end(), TerminalCell{});
        if (mode == 3)
            scrollback_.clear();
        wrap_pending_ = false;
        return;
    }
    if (mode == 0)
    {
        erase_cells(cursor_row_, cursor_col_, cols_);
        for (int r = cursor_row_ + 1; r < rows_; ++r)
            erase_cells(r, 0, cols_);
        return;
    }
    if (mode == 1)
    {
        for (int r = 0; r < cursor_row_; ++r)
            erase_cells(r, 0, cols_);
        erase_cells(cursor_row_, 0, cursor_col_ + 1);
    }
}

void TerminalScreen::erase_in_line(int mode)
{
    if (mode == 2)
    {
        erase_cells(cursor_row_, 0, cols_);
        return;
    }
    if (mode == 0)
    {
        erase_cells(cursor_row_, cursor_col_, cols_);
        return;
    }
    if (mode == 1)
        erase_cells(cursor_row_, 0, cursor_col_ + 1);
}

void TerminalScreen::set_scroll_region(int top, int bottom)
{
    top = std::clamp(top, 0, rows_ - 1);
    bottom = std::clamp(bottom, 0, rows_ - 1);
    if (bottom < top)
        std::swap(top, bottom);
    scroll_top_ = top;
    scroll_bottom_ = bottom;
    set_cursor(scroll_top_, 0);
}

void TerminalScreen::reset_scroll_region()
{
    scroll_top_ = 0;
    scroll_bottom_ = std::max(0, rows_ - 1);
}

void TerminalScreen::index()
{
    if (cursor_row_ == scroll_bottom_)
        scroll_up(1);
    else if (cursor_row_ < rows_ - 1)
        ++cursor_row_;
    wrap_pending_ = false;
}

void TerminalScreen::reverse_index()
{
    if (cursor_row_ == scroll_top_)
        scroll_down(1);
    else if (cursor_row_ > 0)
        --cursor_row_;
    wrap_pending_ = false;
}

TerminalScreen::Line TerminalScreen::make_blank_line() const
{
    return Line(static_cast<std::size_t>(std::max(1, cols_)), TerminalCell{});
}

void TerminalScreen::clamp_cursor()
{
    if (rows_ <= 0 || cols_ <= 0)
    {
        cursor_row_ = 0;
        cursor_col_ = 0;
        return;
    }
    cursor_row_ = std::clamp(cursor_row_, 0, rows_ - 1);
    cursor_col_ = std::clamp(cursor_col_, 0, cols_ - 1);
}

void TerminalScreen::scroll_up(int n)
{
    n = std::max(1, n);
    for (int i = 0; i < n; ++i)
    {
        if (scroll_top_ == 0 && scroll_bottom_ == rows_ - 1)
        {
            push_scrollback(grid_[0]);
            for (int r = 0; r < rows_ - 1; ++r)
                grid_[static_cast<std::size_t>(r)] = std::move(grid_[static_cast<std::size_t>(r + 1)]);
            grid_[static_cast<std::size_t>(rows_ - 1)] = make_blank_line();
        }
        else
        {
            for (int r = scroll_top_; r < scroll_bottom_; ++r)
                grid_[static_cast<std::size_t>(r)] = std::move(grid_[static_cast<std::size_t>(r + 1)]);
            grid_[static_cast<std::size_t>(scroll_bottom_)] = make_blank_line();
        }
    }
}

void TerminalScreen::scroll_down(int n)
{
    n = std::max(1, n);
    for (int i = 0; i < n; ++i)
    {
        for (int r = scroll_bottom_; r > scroll_top_; --r)
            grid_[static_cast<std::size_t>(r)] = std::move(grid_[static_cast<std::size_t>(r - 1)]);
        grid_[static_cast<std::size_t>(scroll_top_)] = make_blank_line();
    }
}

void TerminalScreen::erase_cells(int row, int c0, int c1)
{
    if (row < 0 || row >= rows_)
        return;
    c0 = std::clamp(c0, 0, cols_);
    c1 = std::clamp(c1, 0, cols_);
    if (c0 >= c1)
        return;
    auto &line = grid_[static_cast<std::size_t>(row)];
    for (int c = c0; c < c1; ++c)
        line[static_cast<std::size_t>(c)] = TerminalCell{};
}

