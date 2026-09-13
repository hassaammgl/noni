#include <terminal/terminal_screen.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>

namespace
{
    int glyph_width(char32_t cp)
    {
        if (cp == U'\t')
            return 1; // handled separately
        const int w = TextMetrics::codepoint_width(cp);
        if (w <= 0)
            return 1;
        return w > 2 ? 2 : w;
    }
}

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

void TerminalScreen::put_glyph(char32_t cp, int width)
{
    if (rows_ <= 0 || cols_ <= 0)
        return;
    width = std::clamp(width, 1, 2);

    if (wrap_pending_ && wraparound_)
    {
        wrap_pending_ = false;
        carriage_return();
        index();
    }

    if (cursor_col_ + width > cols_)
    {
        if (wraparound_)
        {
            carriage_return();
            index();
        }
        else
        {
            cursor_col_ = cols_ - width;
            if (cursor_col_ < 0)
                cursor_col_ = 0;
        }
    }

    auto &line = grid_[static_cast<std::size_t>(cursor_row_)];
    TerminalCell cell;
    cell.ch = cp;
    cell.width = static_cast<std::uint8_t>(width);
    cell.attrs = pen_;
    line[static_cast<std::size_t>(cursor_col_)] = cell;

    if (width == 2 && cursor_col_ + 1 < cols_)
    {
        TerminalCell cont;
        cont.ch = 0;
        cont.width = 0;
        cont.attrs = pen_;
        line[static_cast<std::size_t>(cursor_col_ + 1)] = cont;
    }

    cursor_col_ += width;
    if (cursor_col_ >= cols_)
    {
        cursor_col_ = cols_ - 1;
        wrap_pending_ = wraparound_;
    }
}

void TerminalScreen::push_scrollback(Line line)
{
    if (scrollback_limit_ <= 0)
        return;
    if (static_cast<int>(line.size()) != cols_)
        line.resize(static_cast<std::size_t>(cols_), TerminalCell{});
    scrollback_.push_back(std::move(line));
    while (static_cast<int>(scrollback_.size()) > scrollback_limit_)
        scrollback_.pop_front();
}
