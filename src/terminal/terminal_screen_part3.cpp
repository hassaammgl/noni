#include <terminal/terminal_screen.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>

#include "terminal_screen_detail.hpp"

using namespace terminal_screen_detail;

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
