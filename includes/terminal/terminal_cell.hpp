#pragma once

#include <cstdint>

// Terminal cell attributes (ANSI-oriented). Independent of ncurses.
struct TerminalAttrs
{
    // 0–15 = ANSI/bright; 255 = default (theme editor fg/bg).
    std::uint8_t fg = 255;
    std::uint8_t bg = 255;
    bool bold = false;
    bool dim = false;
    bool underline = false;
    bool inverse = false;
};

struct TerminalCell
{
    char32_t ch = U' ';
    // Display width: 1 or 2 for primary glyph; 0 = wide-char continuation cell.
    std::uint8_t width = 1;
    TerminalAttrs attrs{};
};

inline bool cell_is_empty(const TerminalCell &c)
{
    return c.ch == U' ' && c.width == 1 && c.attrs.fg == 255 && c.attrs.bg == 255 &&
           !c.attrs.bold && !c.attrs.dim && !c.attrs.underline && !c.attrs.inverse;
}
