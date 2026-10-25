#pragma once
// P2.3 — buffer view rendering. The PURE part (LineCells + mapping + slicing)
// is unit-tested without a terminal; only draw_buffer_view() touches
// ncurses (ui/ layer, no editing logic).
#include <cstdint>
#include <string>
#include <vector>

#include "core/editor_state.hpp"
#include "core/text.hpp"
#include "ui/screen.hpp"

namespace noni::ui {

// Tab expansion width until the config module lands (P18).
inline constexpr int k_tabwidth = 4;

// Per-codepoint display model of ONE buffer line: what each cp occupies on
// screen (tabs -> blank spans, control/0x7F -> 2-col ^-markers, wide cps ->
// 2 cols, malformed bytes -> one U+FFFD cell). Pure, no ncurses.
class LineCells {
public:
    static LineCells build(const std::string& utf8, int tabwidth);

    struct Span {
        char32_t cp = 0;   // the buffer codepoint
        int width = 1;     // display columns it occupies
        bool marker = false; // rendered as ^X (cp holds the source control)
    };

    [[nodiscard]] std::size_t cps() const { return spans_.size(); }
    [[nodiscard]] int total_width() const;

    // Display column where cp index `cp` starts (cp clamped to count).
    [[nodiscard]] int disp_of_cp(std::uint32_t cp) const;
    // First cp whose span covers display col `disp` (clamps to last cp).
    [[nodiscard]] std::uint32_t cp_of_disp(int disp) const;

    [[nodiscard]] const std::vector<Span>& spans() const { return spans_; }

private:
    std::vector<Span> spans_;
};

// Pure: the literal wide string to draw for the content window
// [left_col, left_col + cols) (cells clipped at the right edge).
[[nodiscard]] std::wstring content_slice(const LineCells& lc, int left_col,
                                         int cols);

// --- screen_row <-> buffer_line (viewport mapping, pure) ---------------------

[[nodiscard]] std::uint32_t screen_row_of_line(std::uint32_t line,
                                               std::uint32_t top_line);
[[nodiscard]] std::uint32_t buffer_line_of_screen_row(int row,
                                                      std::uint32_t top_line);

// Gutter column width: digits of the line count + one padding column (>= 2).
[[nodiscard]] int gutter_width(std::size_t line_count);

// Region of the screen window one (single-)window view owns.
struct ViewArea {
    int top_row = 0;
    int rows = 0;
    int cols = 0;
};

// Draws the ACTIVE window's buffer into `area`: gutter numbers, content
// with horizontal scroll (w.left_col), '~' past EOF, and the cursor line
// highlighted. Terminal cursor is NOT positioned here (P2.5 follows it).
void draw_buffer_view(Screen& screen, EditorState& st, const ViewArea& area);

} // namespace noni::ui
