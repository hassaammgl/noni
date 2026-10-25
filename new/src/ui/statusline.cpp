// P2.4 — see statusline.hpp.
#include "ui/statusline.hpp"

#include <string>

#include <ncurses.h>

#include "core/pos.hpp"

namespace noni::ui {

namespace {

// Mode flag text: always visible in P2 (flips live with i/Esc).
std::wstring mode_flag(Mode mode, bool insert_pending) {
    (void)insert_pending;
    return mode == Mode::Insert ? L"-- INSERT --" : L"-- NORMAL --";
}

std::wstring utf8_to_wide(const std::string& s) {
    // Status strings are latin-ish (paths); decode leniently per codepoint.
    std::wstring out;
    std::size_t i = 0;
    while (i < s.size()) {
        const auto d = decode_utf8(s, i);
        if (!d) {
            out.push_back(L'?');
            ++i;
        } else {
            out.push_back(static_cast<wchar_t>(d->first));
            i += d->second;
        }
    }
    return out;
}

std::wstring file_label(const EditorState& st) {
    const WindowState& w = st.win();
    const Buffer* b = st.buffer(w.buffer);
    if (b == nullptr) {
        return L"[no window]";
    }
    if (b->path().empty()) {
        return L"[No Name]";
    }
    return utf8_to_wide(b->path().filename().string());
}

} // namespace

void draw_layout(Screen& screen, EditorState& st) {
    WINDOW* win = screen.win();
    const int rows = screen.rows();
    const int cols = screen.cols();
    wclear(win);

    // Tab-bar placeholder (row 0): colored at P8; v0 leaves it blank.
    mvwprintw(win, 0, 0, "%s", "");

    // View region.
    ViewArea area;
    area.top_row = 1;
    area.rows = rows - 3;
    area.cols = cols;
    if (area.rows > 0) {
        draw_buffer_view(screen, st, area);
    }

    // Statusline.
    const int srow = rows - 2;
    const WindowState& w = st.win();
    const Buffer* b = st.buffer(w.buffer);
    wattron(win, A_REVERSE);
    std::wstring text = mode_flag(st.mode, false);
    text += L"  ";
    text += file_label(st);
    if (b != nullptr && b->dirty()) {
        text += L" [+]";
    }
    {
        const std::wstring right = std::wstring(L"Ln ") +
            std::to_wstring(w.cursor.line + 1) + L", Col " +
            std::to_wstring(w.cursor.col + 1);
        std::wstring row(cols, L' ');
        for (std::size_t i = 0; i < text.size()
                                  && i < static_cast<std::size_t>(cols);
             ++i) {
            row[i] = text[i];
        }
        if (text.size() + right.size() + 1
                < static_cast<std::size_t>(cols)) {
            const std::size_t off = static_cast<std::size_t>(cols)
                                  - right.size() - 1;
            for (std::size_t i = 0; i < right.size(); ++i) {
                row[off + i] = right[i];
            }
        }
        mvwaddnwstr(win, srow, 0, row.c_str(), cols);
    }
    wattroff(win, A_REVERSE);

    // Message / cmdline row (reserved; the prompt content is P6).
    if (!st.msg.empty()) {
        const std::wstring msg = utf8_to_wide(st.msg);
        mvwaddnwstr(win, rows - 1, 0, msg.c_str(),
                    static_cast<int>(msg.size())
                        < cols ? static_cast<int>(msg.size()) : cols);
    }

    wrefresh(win);
}

} // namespace noni::ui
