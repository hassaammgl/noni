// P2.3 — see view.hpp.
#include "ui/view.hpp"

#include <cwchar>

#include <ncurses.h>

#include "core/pos.hpp"

namespace noni::ui {

LineCells LineCells::build(const std::string& utf8, int tabwidth) {
    LineCells lc;
    int disp = 0;
    std::size_t byte = 0;
    while (byte < utf8.size()) {
        Span s{};
        const auto d = decode_utf8(utf8, byte);
        if (!d) {
            s.cp = 0xFFFD; // malformed byte: one pseudo-cell (P1.1 lenient)
            s.width = 1;
            byte += 1;
        } else {
            const char32_t c = d->first;
            byte += d->second;
            if (c == U'\t') {
                const int pad = tabwidth - (disp % tabwidth);
                s.cp = U'\t';
                s.width = pad < 1 ? 1 : pad;
            } else if (c < 0x20 || c == 0x7F) {
                s.cp = c;
                s.width = 2; // ^A style marker
                s.marker = true;
            } else {
                s.cp = c;
                s.width = cp_display_width(c, tabwidth, disp);
            }
        }
        lc.spans_.push_back(s);
        disp += s.width;
    }
    return lc;
}

int LineCells::total_width() const {
    int w = 0;
    for (const Span& s : spans_) {
        w += s.width;
    }
    return w;
}

int LineCells::disp_of_cp(std::uint32_t cp) const {
    if (spans_.empty()) {
        return 0;
    }
    if (cp >= spans_.size()) {
        cp = static_cast<std::uint32_t>(spans_.size() - 1);
    }
    int disp = 0;
    for (std::size_t i = 0; i < cp; ++i) {
        disp += spans_[i].width;
    }
    return disp;
}

std::uint32_t LineCells::cp_of_disp(int disp) const {
    if (spans_.empty() || disp < 0) {
        return 0;
    }
    int d = 0;
    for (std::size_t i = 0; i < spans_.size(); ++i) {
        d += spans_[i].width;
        if (disp < d) {
            return static_cast<std::uint32_t>(i);
        }
    }
    return static_cast<std::uint32_t>(spans_.size() - 1);
}

std::wstring content_slice(const LineCells& lc, int left_col, int cols) {
    std::wstring out;
    if (cols <= 0) {
        return out;
    }
    int pos = 0;   // display col of the current cell start
    int out_width = 0; // display width of the emitted glyphs
    for (const auto& s : lc.spans()) {
        const int end = pos + s.width;
        if (end <= left_col) { // fully scrolled off the left
            pos = end;
            continue;
        }
        if (pos >= left_col + cols) { // beyond the right edge
            break;
        }
        // Rendering: control markers expand to '^X'/'^?'; tabs blank out.
        // Wide glyphs need BOTH their columns on screen (P2.8): a wide span
        // straddling either scroll edge renders blank cells, otherwise the
        // terminal consumes 2 cells and shifts the row alignment. Markers
        // and tabs are per-column self-sufficient and keep their clipping.
        std::string glyphs;
        if (s.marker) {
            glyphs += '^';
            glyphs += static_cast<char>(s.cp == 0x7F ? '?' : s.cp + '@');
        }
        const bool wide_partial = s.width == 2 && !s.marker && s.cp != U'\t'
                                  && (pos < left_col || end > left_col + cols);
        const std::wstring cell = [&] {
            std::wstring w;
            if (s.marker) {
                for (char g : glyphs) {
                    w.push_back(static_cast<wchar_t>(
                        static_cast<unsigned char>(g)));
                }
                return w;
            }
            if (s.cp == U'\t') {
                w.assign(static_cast<std::size_t>(s.width),
                         static_cast<wchar_t>(U' '));
                return w;
            }
            if (wide_partial) {
                return std::wstring();
            }
            w.assign(static_cast<std::size_t>(1),
                     static_cast<wchar_t>(s.cp));
            return w;
        }();
        // Clip the span at both edges. Rendering units:
        //  - markers/tabs/blanks: one glyph per DISPLAY column;
        //  - wide glyphs: one element, the terminal consumes 2 cells (no
        //    filler after it); partial-wide spans render blank columns.
        for (int i = 0; i < s.width; ++i) {
            const int col = pos + i;
            if (col < left_col || col >= left_col + cols) {
                continue;
            }
            const bool wide_glyph = s.width == 2 && !s.marker
                                    && s.cp != U'\t' && !wide_partial;
            if (wide_glyph) {
                if (i == 0) {
                    out.push_back(cell[0]);
                    out_width += s.width;
                }
                continue;
            }
            if (i < static_cast<int>(cell.size())) {
                out.push_back(cell[static_cast<std::size_t>(i)]);
            } else {
                out.push_back(L' '); // tail of a partially spanned cell
            }
            ++out_width;
        }
        pos = end;
    }
    // Row-fill: blanks until the slice covers `cols` DISPLAY columns so the
    // highlight fills the whole row (wide glyphs made some cells 2-wide).
    while (out_width < cols) {
        out.push_back(L' ');
        ++out_width;
    }
    return out;
}

std::uint32_t screen_row_of_line(std::uint32_t line, std::uint32_t top_line) {
    return line >= top_line ? line - top_line : 0;
}

std::uint32_t buffer_line_of_screen_row(int row, std::uint32_t top_line) {
    return row <= 0 ? top_line : top_line + static_cast<std::uint32_t>(row);
}

bool is_past_eof_row(const int row, const std::uint32_t top_line,
                     const std::size_t line_count) {
    const std::uint32_t line = buffer_line_of_screen_row(row, top_line);
    return line >= line_count;
}

int gutter_width(std::size_t line_count) {
    int digits = 1;
    for (std::size_t n = line_count; n >= 10; n /= 10) {
        ++digits;
    }
    return digits + 1;
}

namespace {

void add_wstr_at(WINDOW* win, int row, int col, const std::wstring& text) {
    if (text.empty()) {
        return;
    }
    mvwaddnwstr(win, row, col, text.c_str(), static_cast<int>(text.size()));
}

} // namespace

void draw_buffer_view(Screen& screen, const EditorState& st, const ViewArea& area) {
    WINDOW* win = screen.win();
    const WindowState& w = st.win();
    const Buffer* b = st.buffer(w.buffer);
    const int text_cols = area.cols - gutter_width(b ? b->lines().size() : 1);
    if (b == nullptr || area.rows <= 0 || text_cols <= 0) {
        return;
    }
    const auto& lines = b->lines();
    const std::uint32_t cursor_line = w.cursor.line
                                          < lines.size()
                                          ? w.cursor.line
                                          : 0;

    for (int row = 0; row < area.rows; ++row) {
        if (is_past_eof_row(row, w.top_line, lines.size())) {
            mvwaddnwstr(win, area.top_row + row, 0, L"~", 1);
            wclrtoeol(win);
            continue;
        }
        const std::uint32_t line = buffer_line_of_screen_row(row, w.top_line);
        const bool highlighted = (line == cursor_line);
        if (highlighted) {
            wattron(win, A_REVERSE);
        }
        // Gutter number, right-aligned.
        std::string num = std::to_string(line + 1);
        while (num.size() < static_cast<std::size_t>(gutter_width(lines.size())
                                                        - 1)) {
            num.insert(num.begin(), ' ');
        }
        num.push_back(' ');
        std::wstring gw(num.begin(), num.end());
        add_wstr_at(win, area.top_row + row, 0, gw);

        // Content clip [left_col, left_col + text_cols). The slice is
        // blank-filled to full width, so A_REVERSE covers the whole row.
        const LineCells lc = LineCells::build(lines[line], k_tabwidth);
        const std::wstring slice =
            content_slice(lc, static_cast<int>(w.left_col), text_cols);
        add_wstr_at(win, area.top_row + row, gutter_width(lines.size()),
                    slice);
        // Pad the highlight across the full row on the cursor line.
        if (highlighted) {
            wclrtoeol(win);
            wattroff(win, A_REVERSE);
        }
    }
    // Above-EOF rows are drawn lazily: rows past the window height don't
    // exist on screen, so old content can linger only inside `area`.
    // (The main loop full-clears each frame at P2.4; no leak here.)
}

} // namespace noni::ui
