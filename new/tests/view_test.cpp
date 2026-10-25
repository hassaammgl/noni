#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "ui/view.hpp"

using namespace noni;
using namespace noni::ui;

namespace {
LineCells build(const std::string& s) { return LineCells::build(s, k_tabwidth); }
} // namespace

TEST_CASE("view: plain ascii line maps cp<->disp 1:1") {
    const LineCells lc = build("abc");
    CHECK(lc.cps() == 3);
    CHECK(lc.total_width() == 3);
    CHECK(lc.disp_of_cp(0) == 0);
    CHECK(lc.disp_of_cp(1) == 1);
    CHECK(lc.disp_of_cp(2) == 2);
    CHECK(lc.cp_of_disp(0) == 0);
    CHECK(lc.cp_of_disp(2) == 2);
    CHECK(lc.cp_of_disp(9) == 2); // clamp past end
    CHECK(lc.cp_of_disp(-1) == 0);
}

TEST_CASE("view: tab expands to the next stop") {
    const LineCells lc = build("a\tb"); // tabwidth 4
    CHECK(lc.cps() == 3);
    CHECK(lc.total_width() == 5); // a(1) + tab(3) + b(1)
    CHECK(lc.disp_of_cp(0) == 0); // 'a' at 0
    CHECK(lc.disp_of_cp(1) == 1); // tab span starts at 1, width 3
    CHECK(lc.disp_of_cp(2) == 4); // 'b' at col 4
    CHECK(lc.cp_of_disp(1) == 1); // cover disp 1..3 -> the TAB
    CHECK(lc.cp_of_disp(3) == 1);
    CHECK(lc.cp_of_disp(4) == 2); // 'b'
    SUBCASE("tab at column 0 is a full stop") {
        const LineCells t = build("\t");
        CHECK(t.total_width() == 4);
    }
    SUBCASE("tab aligns to the next stop from disp 1") {
        const LineCells t = build("x\ty"); // pad 4-1=3 -> 'y' at disp 4
        CHECK(t.disp_of_cp(2) == 4);
        CHECK(t.cp_of_disp(1) == 1);
        CHECK(t.cp_of_disp(3) == 1);
    }
}

TEST_CASE("view: wide cps take two columns") {
    const LineCells lc = build("一a"); // CJK wide + ascii
    CHECK(lc.cps() == 2);
    CHECK(lc.total_width() == 3);
    CHECK(lc.disp_of_cp(1) == 2);
    CHECK(lc.cp_of_disp(1) == 0); // inside the wide cell
    CHECK(lc.cp_of_disp(2) == 1);
}

TEST_CASE("view: control cps and malformed bytes marker-expand") {
    const LineCells lc = build(std::string("\x01\x7f\xff", 3));
    CHECK(lc.cps() == 3);
    // ^A (2 cols) + ^? (2 cols) + U+FFFD replacement (1 col)
    CHECK(lc.total_width() == 5);
    CHECK(lc.disp_of_cp(2) == 4);
    // Both columns of a 2-col marker map onto the SAME cp.
    CHECK(lc.cp_of_disp(0) == 0);
    CHECK(lc.cp_of_disp(1) == 0); // second col of ^A
    CHECK(lc.cp_of_disp(4) == 2);
}

TEST_CASE("view: content_slice clips with horizontal scroll") {
    // Slice contract: exactly `cols` columns, blanks after EOL (v0 row fill).
    SUBCASE("ascii, no scroll") {
        CHECK(content_slice(build("hello"), 0, 3) == L"hel");
        const std::wstring s = content_slice(build("hello"), 2, 10);
        CHECK(s.compare(0, 3, L"llo") == 0);
        CHECK(s.size() == 10);
        CHECK(s.back() == L' ');
        CHECK(content_slice(build("hi"), 0, 2) == L"hi");
    }
    SUBCASE("tab scrolled off the left blanks partially") {
        const LineCells lc = build("a\tb");
        // disp: a=0, tab span=1..3, b=4
        const std::wstring s1 = content_slice(lc, 2, 10);
        CHECK(s1[0] == L' '); // half of the tab blank span
        CHECK(s1[2] == L'b'); // wait — the b starts at disp 4 here
        CHECK(s1.size() == 10);
        const std::wstring s2 = content_slice(lc, 0, 2);
        CHECK(s2[0] == L'a');
        CHECK(s2[1] == L' ');
    }
    SUBCASE("control marker partially scrolled keeps its glyphs") {
        const LineCells lc = build(std::string{'\x01', 'b'}); // ^A 0..1, b 2
        const std::wstring s1 = content_slice(lc, 1, 4);
        CHECK(s1[0] == L'A'); // tail of the ^A marker
        CHECK(s1[1] == L'b');
        CHECK(s1.size() == 4); // blank-filled to the requested width
    }
    SUBCASE("zero width yields empty; past-EOL scroll is blanks") {
        CHECK(content_slice(build("x"), 0, 0).empty());
        const std::wstring past = content_slice(build("x"), 5, 10);
        CHECK(past.size() == 10);
        CHECK(past[0] == L' ');
    }
}

TEST_CASE("view: viewport mapping both ways") {
    CHECK(screen_row_of_line(3, 1) == 2);
    CHECK(screen_row_of_line(1, 5) == 0); // scrolled above: clamps 0
    CHECK(buffer_line_of_screen_row(0, 5) == 5);
    CHECK(buffer_line_of_screen_row(4, 5) == 9);
    CHECK(buffer_line_of_screen_row(-1, 5) == 5); // row<0 clamps to top
}

TEST_CASE("view: P2.8 past-eof rows and the exactly-filling viewport") {
    // Empty buffer: ONE content row (the empty line), all else '~'.
    CHECK(!is_past_eof_row(0, 0, 1));
    CHECK(is_past_eof_row(1, 0, 1));
    CHECK(is_past_eof_row(4, 0, 1));

    // Viewport exactly filled: 30 lines, top 25, 5 rows -> no '~' lane,
    // the last screen row shows the LAST line (half-visible-bottom edge).
    for (int row = 0; row < 5; ++row) {
        CHECK(!is_past_eof_row(row, 25, 30));
    }
    CHECK(buffer_line_of_screen_row(4, 25) == 29);

    // Half-visible bottom: top 26 -> row 4 lands past EOF ('~' lane).
    CHECK(!is_past_eof_row(3, 26, 30));
    CHECK(is_past_eof_row(4, 26, 30));
    // Top 24 on a SIX-row viewport still shows the last line on row 5 —
    // a viewport taller than the remaining lines shows '~' from row 6 up.
    CHECK(buffer_line_of_screen_row(5, 24) == 29);
    CHECK(!is_past_eof_row(5, 24, 30));
    CHECK(is_past_eof_row(6, 24, 30));
}

TEST_CASE("view: P2.8 slice edge cases (wide chars, exact right edge)") {
    SUBCASE("line ending exactly at the right edge") {
        const std::wstring s = content_slice(build("hello"), 0, 5);
        CHECK(s == L"hello");
        CHECK(s.size() == 5); // no clipping spill, no fill needed
    }
    SUBCASE("wide char straddling the RIGHT edge blanks out") {
        // y(0) 一(1..2) z(3): cols requested 0..1 -> the wide glyph is
        // half visible; must render blanks, NOT the glyph (alignment).
        const std::wstring s = content_slice(build("y一z"), 0, 2);
        CHECK(s.size() == 2);
        CHECK(s[0] == L'y');
        CHECK(s[1] == L' ');
        // Fully visible wide glyph renders normally.
        const std::wstring w3 = content_slice(build("y一z"), 0, 4);
        CHECK(w3[0] == L'y');
        CHECK(w3[1] == L'一');
        CHECK(w3[2] == L'z');
    }
    SUBCASE("wide char straddling the LEFT edge blanks its tail") {
        const std::wstring s = content_slice(build("一x"), 1, 4);
        CHECK(s.size() == 4);
        CHECK(s[0] == L' '); // second half of the wide cell: blank
        CHECK(s[1] == L'x');
    }
    SUBCASE("empty line content slice is all blanks of the window width") {
        const std::wstring s = content_slice(build(""), 0, 4);
        CHECK(s.size() == 4);
        CHECK(s == L"    ");
    }
}

TEST_CASE("view: gutter width is digits + one pad") {
    CHECK(gutter_width(1) == 2);
    CHECK(gutter_width(9) == 2);
    CHECK(gutter_width(10) == 3);
    CHECK(gutter_width(999) == 4);
    CHECK(gutter_width(1000) == 5);
}
