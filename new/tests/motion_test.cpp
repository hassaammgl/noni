#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

// P2.7 — differential fixture test: replays the recorded nvim oracle keys
// (tests/diff/motion_p2.fxt) through the REAL app path (dispatch + reducer)
// and asserts the end cursor/text matches line for line. Fixture format:
//   name keys startLine,startCol endLine,endCol | line1|line2...
// (1-based cols in the file; <Esc> is the readable Esc token; empty-picture
// lines come through as "".) Regenerate with tests/diff/gen_motion.vim —
// never hand-edit expected values (AGENTS differential rule).
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "app/reducer.hpp"
#include "core/editor_state.hpp"
#include "core/pos.hpp"
#include "core/view.hpp"

using namespace noni;

namespace {

struct OracleCase {
    std::string name;
    std::string keys;
    Pos start{};
    Pos want{};
    std::vector<std::string> lines;       // starting content (init picture)
    std::vector<std::string> final_lines; // expected end content
    // P3.6 only: the expected end topline (1-based, as nvim records it; 0 =
    // the fixture has no view expectation).
    std::uint32_t want_top = 0;
};

// Splits a '|'-joined line vector ("" -> one empty line).
std::vector<std::string> split_pict(const std::string& pict) {
    std::vector<std::string> out;
    std::size_t begin = 0;
    for (;;) {
        const auto at = pict.find('|', begin);
        if (at == std::string::npos) {
            out.push_back(pict.substr(begin));
            break;
        }
        out.push_back(pict.substr(begin, at - begin));
        begin = at + 1;
    }
    return out;
}

// "5,7" file coords -> 0-based {4,6}.
Pos parse_lc(const std::string& s) {
    const auto comma = s.find(',');
    return Pos{static_cast<std::uint32_t>(std::stoul(s.substr(0, comma))) - 1,
               static_cast<std::uint32_t>(std::stoul(s.substr(comma + 1)))
                   - 1};
}

std::optional<OracleCase> parse_line(const std::string& line) {
    if (line.empty() || line.rfind("fixture:", 0) == 0) {
        return std::nullopt;
    }
    const auto sep = line.find(" | ");
    if (sep == std::string::npos) {
        return std::nullopt; // malformed: skipped; count-checked in the test
    }
    OracleCase c;
    std::istringstream is(line.substr(0, sep));
    is >> c.name >> c.keys;
    std::string slc, elc;
    is >> slc >> elc;
    c.start = parse_lc(slc);
    c.want = parse_lc(elc);
    // P3.6 fixtures append `top=<end topline>` (1-based); older fixtures do
    // not, so every other token is ignored.
    for (std::string extra; is >> extra;) {
        if (extra.rfind("top=", 0) == 0) {
            c.want_top = static_cast<std::uint32_t>(
                std::stoul(extra.substr(4)));
        }
    }

    // Pictures: "| <init> @ <final>", both '|'-joined line vectors.
    std::string both = line.substr(sep + 3);
    const auto divider = both.find(" @ ");
    if (divider == std::string::npos) {
        return std::nullopt;
    }
    c.lines = split_pict(both.substr(0, divider));       // starting content
    c.final_lines = split_pict(both.substr(divider + 3)); // expected result
    return c;
}

// Keys replay: '<Esc>' becomes KeyKind::Esc, '<SPC>' a space Char (the
// fixture records are space-delimited, so a space target needs a token);
// '<C-x>' the control key (P3.6 scrolls); any other byte is a Char.
std::vector<Key> decode_keys(const std::string& s) {
    std::vector<Key> keys;
    for (std::size_t i = 0; i < s.size();) {
        if (s.compare(i, 5, "<Esc>") == 0) {
            keys.push_back(Key{KeyKind::Esc, 0});
            i += 5;
        } else if (s.compare(i, 5, "<SPC>") == 0) {
            keys.push_back(Key{KeyKind::Char, U' '});
            i += 5;
        } else if (s.compare(i, 5, "<C-f>") == 0) {
            keys.push_back(Key{KeyKind::CtrlChar, U'f'});
            i += 5;
        } else if (s.compare(i, 5, "<C-b>") == 0) {
            keys.push_back(Key{KeyKind::CtrlChar, U'b'});
            i += 5;
        } else if (s.compare(i, 5, "<C-d>") == 0) {
            keys.push_back(Key{KeyKind::CtrlChar, U'd'});
            i += 5;
        } else if (s.compare(i, 5, "<C-u>") == 0) {
            keys.push_back(Key{KeyKind::CtrlChar, U'u'});
            i += 5;
        } else {
            keys.push_back(Key{KeyKind::Char, static_cast<char32_t>(s[i])});
            ++i;
        }
    }
    return keys;
}

std::optional<std::string> find_fixture(const std::string& name) {
    // ctest runs from the build dir; fixtures live in the source tree.
    const std::string candidates[] = {"../tests/diff/" + name,
                                      "tests/diff/" + name,
                                      "../../tests/diff/" + name};
    for (const std::string& cand : candidates) {
        std::ifstream probe(cand);
        if (probe.good()) {
            return cand;
        }
    }
    return std::nullopt;
}

// One scripted keystroke: translate then reduce (the P2.5 loop body).
void key(EditorState& st, const Key& k) {
    noni::app::reducer(st, noni::app::command_for_key(st, k));
}

} // namespace

TEST_CASE("motion: differential replay of the P2 nvim oracle fixture") {
    const auto path = find_fixture("motion_p2.fxt");
    REQUIRE_MESSAGE(path.has_value(),
                    "tests/diff/motion_p2.fxt not found (build dir relative)");
    std::ifstream in(*path);
    REQUIRE(in.good());

    std::vector<OracleCase> cases;
    std::string line;
    while (std::getline(in, line)) {
        if (auto c = parse_line(line)) {
            cases.push_back(*c);
        }
    }
    REQUIRE_MESSAGE(cases.size() >= 25, "fixture parsed too few oracle cases");

    for (const OracleCase& c : cases) {
        INFO("oracle case: ", c.name, "  keys: ", c.keys);
        EditorState st;
        st.buffers[0].set_lines(c.lines);
        st.win().cursor = c.start;
        // Vim starts every player-visible cursor position with
        // curswant == col; our fixture replay honours that invariant.
        st.win().desired_col = c.start.col;
        st.mode = Mode::Normal;

        for (const Key& k : decode_keys(c.keys)) {
            key(st, k);
        }

        CHECK(st.win().cursor == c.want);
        CHECK(st.buffers[0].lines() == c.final_lines);
    }
}

TEST_CASE("motion: differential replay of the P3.3 find fixture") {
    const auto path = find_fixture("motion_p3c.fxt");
    REQUIRE_MESSAGE(path.has_value(),
                    "tests/diff/motion_p3c.fxt not found (build dir relative)");
    std::ifstream in(*path);
    REQUIRE(in.good());

    std::vector<OracleCase> cases;
    std::string line;
    while (std::getline(in, line)) {
        if (auto c = parse_line(line)) {
            cases.push_back(*c);
        }
    }
    REQUIRE_MESSAGE(cases.size() >= 26, "fixture parsed too few oracle cases");

    for (const OracleCase& c : cases) {
        INFO("oracle case: ", c.name, "  keys: ", c.keys);
        EditorState st;
        st.buffers[0].set_lines(c.lines);
        st.win().cursor = c.start;
        st.win().desired_col = c.start.col;
        st.mode = Mode::Normal;

        for (const Key& k : decode_keys(c.keys)) {
            key(st, k);
        }

        CHECK(st.win().cursor == c.want);
        CHECK(st.buffers[0].lines() == c.final_lines);
    }
}

TEST_CASE("motion: find motions fail through eresult (P3.3)") {
    const std::vector<std::string> lines{"ab a b", "zz"};

    // No occurrence right of the col -> error, nothing moves.
    {
        const Motion m{MotionId::FindFwd, U'z'};
        const auto r = m.apply({&lines, k_default_tabwidth}, Pos{0, 0}, 1, 0);
        REQUIRE(!r.has_value());
        CHECK(r.error().msg.size() > 0);
    }
    // Count-N with an N-1 short line fails WHOLE (oracle: 3fa never
    // partially moves). A later `;` repeats and fails the same way.
    {
        const Motion m{MotionId::FindFwd, U'a'};
        const auto r = m.apply({&lines, k_default_tabwidth}, Pos{0, 0}, 4, 0);
        REQUIRE(!r.has_value());
    }
    // 't' with the target exactly adjacent: success, landing = cursor.
    {
        const std::vector<std::string> one{"ax"};
        const Motion m{MotionId::ToFwd, U'x'};
        const auto r = m.apply({&one, k_default_tabwidth}, Pos{0, 0}, 1, 0);
        REQUIRE(r.has_value());
        CHECK(r->line == 0);
        CHECK(r->col == 0);
    }
    // 'T' landing: one char AFTER the backward occurrence (col1 'b' -> col2).
    {
        const Motion m{MotionId::ToBack, U'b'};
        const auto r = m.apply({&lines, k_default_tabwidth}, Pos{0, 4}, 1, 0);
        REQUIRE(r.has_value());
        CHECK(r->line == 0);
        CHECK(r->col == 2);
    }
    // '^': first non-blank; blank-only line rests on the LAST char;
    // count is ignored (P3.4).
    {
        const std::vector<std::string> ls{"  hello", "   ", ""};
        Motion m{MotionId::FirstNonblank};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{0, 4}, 1, 0);
        REQUIRE(r.has_value());
        CHECK(r->line == 0);
        CHECK(r->col == 2);
        const auto rb = m.apply({&ls, k_default_tabwidth}, Pos{1, 1}, 9, 0);
        REQUIRE(rb.has_value());
        CHECK(rb->line == 1);
        CHECK(rb->col == 2);
        const auto re = m.apply({&ls, k_default_tabwidth}, Pos{2, 0}, 1, 0);
        REQUIRE(re.has_value());
        CHECK(re->col == 0);
    }
    // gg/G: col carried (vertical semantics); counts clamp (P3.4).
    {
        const std::vector<std::string> ls{"one", "two", "  three"};
        Motion g{MotionId::GotoLast};
        const auto r1 = g.apply({&ls, k_default_tabwidth}, Pos{0, 1}, 1, 1);
        REQUIRE(r1.has_value());
        CHECK(r1->line == 2);
        CHECK(r1->col == 1);
        Motion gg{MotionId::GotoFirst};
        // 12gg on a 3-line file clamps to the LAST line, keeping the col.
        const auto r2 = gg.apply({&ls, k_default_tabwidth}, Pos{2, 1}, 12, 1);
        REQUIRE(r2.has_value());
        CHECK(r2->line == 2);
        CHECK(r2->col == 1);
    }
}

TEST_CASE("motion: differential replay of the P3.5 paragraph fixture") {
    const auto path = find_fixture("motion_p3e.fxt");
    REQUIRE_MESSAGE(path.has_value(),
                    "tests/diff/motion_p3e.fxt not found (build dir relative)");
    std::ifstream in(*path);
    REQUIRE(in.good());

    std::vector<OracleCase> cases;
    std::string line;
    while (std::getline(in, line)) {
        if (auto c = parse_line(line)) {
            cases.push_back(*c);
        }
    }
    REQUIRE_MESSAGE(cases.size() >= 22, "fixture parsed too few oracle cases");

    for (const OracleCase& c : cases) {
        INFO("oracle case: ", c.name, "  keys: ", c.keys);
        EditorState st;
        st.buffers[0].set_lines(c.lines);
        st.win().cursor = c.start;
        st.win().desired_col = c.start.col;
        st.mode = Mode::Normal;

        for (const Key& k : decode_keys(c.keys)) {
            key(st, k);
        }

        CHECK(st.win().cursor == c.want);
        CHECK(st.buffers[0].lines() == c.final_lines);
    }
}

TEST_CASE("motion: differential replay of the P3.4 caret/gg/G fixture") {
    const auto path = find_fixture("motion_p3d.fxt");
    REQUIRE_MESSAGE(path.has_value(),
                    "tests/diff/motion_p3d.fxt not found (build dir relative)");
    std::ifstream in(*path);
    REQUIRE(in.good());

    std::vector<OracleCase> cases;
    std::string line;
    while (std::getline(in, line)) {
        if (auto c = parse_line(line)) {
            cases.push_back(*c);
        }
    }
    REQUIRE_MESSAGE(cases.size() >= 28, "fixture parsed too few oracle cases");

    for (const OracleCase& c : cases) {
        INFO("oracle case: ", c.name, "  keys: ", c.keys);
        EditorState st;
        st.buffers[0].set_lines(c.lines);
        st.win().cursor = c.start;
        st.win().desired_col = c.start.col;
        st.mode = Mode::Normal;

        for (const Key& k : decode_keys(c.keys)) {
            key(st, k);
        }

        CHECK(st.win().cursor == c.want);
        CHECK(st.buffers[0].lines() == c.final_lines);
    }
}

TEST_CASE("motion: differential replay of the P3.2 b/B/e/E fixture") {
    const auto path = find_fixture("motion_p3b.fxt");
    REQUIRE_MESSAGE(path.has_value(),
                    "tests/diff/motion_p3b.fxt not found (build dir relative)");
    std::ifstream in(*path);
    REQUIRE(in.good());

    std::vector<OracleCase> cases;
    std::string line;
    while (std::getline(in, line)) {
        if (auto c = parse_line(line)) {
            cases.push_back(*c);
        }
    }
    REQUIRE_MESSAGE(cases.size() >= 40, "fixture parsed too few oracle cases");

    for (const OracleCase& c : cases) {
        INFO("oracle case: ", c.name, "  keys: ", c.keys);
        EditorState st;
        st.buffers[0].set_lines(c.lines);
        st.win().cursor = c.start;
        st.win().desired_col = c.start.col;
        st.mode = Mode::Normal;

        for (const Key& k : decode_keys(c.keys)) {
            key(st, k);
        }

        CHECK(st.win().cursor == c.want);
        CHECK(st.buffers[0].lines() == c.final_lines);
    }
}
TEST_CASE("motion: differential replay of the P3 word-motion fixture") {
    const auto path = find_fixture("motion_p3.fxt");
    REQUIRE_MESSAGE(path.has_value(),
                    "tests/diff/motion_p3.fxt not found (build dir relative)");
    std::ifstream in(*path);
    REQUIRE(in.good());

    std::vector<OracleCase> cases;
    std::string line;
    while (std::getline(in, line)) {
        if (auto c = parse_line(line)) {
            cases.push_back(*c);
        }
    }
    REQUIRE_MESSAGE(cases.size() >= 27, "fixture parsed too few oracle cases");

    for (const OracleCase& c : cases) {
        INFO("oracle case: ", c.name, "  keys: ", c.keys);
        EditorState st;
        st.buffers[0].set_lines(c.lines);
        st.win().cursor = c.start;
        st.win().desired_col = c.start.col;
        st.mode = Mode::Normal;

        for (const Key& k : decode_keys(c.keys)) {
            key(st, k);
        }

        CHECK(st.win().cursor == c.want);
        CHECK(st.buffers[0].lines() == c.final_lines);
    }
}

// --- P3.6: viewport scrolls ------------------------------------------------

TEST_CASE("motion: differential replay of the P3.6 scroll fixture") {
    const auto path = find_fixture("motion_p3f.fxt");
    REQUIRE_MESSAGE(path.has_value(),
                    "tests/diff/motion_p3f.fxt not found (build dir relative)");
    std::ifstream in(*path);
    REQUIRE(in.good());

    std::vector<OracleCase> cases;
    std::string line;
    while (std::getline(in, line)) {
        if (auto c = parse_line(line)) {
            cases.push_back(*c);
        }
    }
    REQUIRE_MESSAGE(cases.size() >= 40, "fixture parsed too few oracle cases");

    // The oracle window was 22 rows tall (nvim &lines=24 minus chrome).
    constexpr std::uint32_t kRows = 22;
    for (const OracleCase& c : cases) {
        INFO("oracle case: ", c.name, "  keys: ", c.keys);
        EditorState st;
        st.buffers[0].set_lines(c.lines);
        st.win().cursor = c.start;
        st.win().desired_col = c.start.col;
        st.win().view_rows = static_cast<int>(kRows);
        // The generator starts every case with the cursor on the last visible
        // row: vim's own state after jumping there with scrolloff 0.
        st.win().top_line =
            c.start.line > kRows - 1 ? c.start.line - (kRows - 1) : 0;
        st.mode = Mode::Normal;

        for (const Key& k : decode_keys(c.keys)) {
            key(st, k);
            // The real App renders (and follows the cursor) after every key,
            // which is also what nvim does; replay must do the same or the
            // follow-up scroll keys would see a stale viewport.
            app::follow_cursor(st, static_cast<int>(kRows));
        }

        CHECK(st.win().cursor == c.want);
        if (c.want_top > 0) {
            CHECK(st.win().top_line == c.want_top - 1);
        }
        CHECK(st.buffers[0].lines() == c.final_lines);
    }
}

TEST_CASE("view: H/M/L pick visible rows and clamp (P3.6)") {
    const ViewGeometry g{40, 22, 0};

    const auto apply = [&](ScrollId id, std::uint32_t cur, std::uint32_t top,
                           std::uint32_t count, bool counted) {
        return apply_scroll(g, ScrollRequest{id, cur, top, count, counted});
    };

    SUBCASE("H counts from the top, L from the bottom") {
        CHECK(apply(ScrollId::LineTop, 9, 0, 1, false).cursor_line == 0);
        CHECK(apply(ScrollId::LineTop, 9, 0, 3, true).cursor_line == 2);
        CHECK(apply(ScrollId::LineBottom, 9, 0, 1, false).cursor_line == 21);
        CHECK(apply(ScrollId::LineBottom, 9, 0, 2, true).cursor_line == 20);
    }
    SUBCASE("M ignores the count and centres the VISIBLE lines") {
        CHECK(apply(ScrollId::LineMiddle, 9, 0, 1, false).cursor_line == 10);
        CHECK(apply(ScrollId::LineMiddle, 9, 0, 5, true).cursor_line == 10);
        // A 10-line buffer in a 22-row view: middle is line 4, not row 10.
        const ViewGeometry short_buf{10, 22, 0};
        const auto r = apply_scroll(short_buf,
                                    ScrollRequest{ScrollId::LineMiddle, 2, 0,
                                                  1, false});
        CHECK(r.cursor_line == 4);
    }
    SUBCASE("counts past the visible area clamp to the view edges") {
        CHECK(apply(ScrollId::LineTop, 9, 0, 30, true).cursor_line == 21);
        CHECK(apply(ScrollId::LineBottom, 9, 0, 30, true).cursor_line == 0);
    }
    SUBCASE("H/M/L never move the viewport") {
        CHECK(apply(ScrollId::LineTop, 20, 5, 1, false).top == 5);
        CHECK(apply(ScrollId::LineMiddle, 20, 5, 1, false).top == 5);
        CHECK(apply(ScrollId::LineBottom, 20, 5, 1, false).top == 5);
    }
}

TEST_CASE("view: zz/zt/zb geometry (P3.6)") {
    const ViewGeometry g{40, 22, 0};

    SUBCASE("zz centres the cursor line, cursor stays") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::Center, 19, 0,
                                                     1, false});
        CHECK(r.top == 9); // 19 - (22-1)/2
        CHECK(r.cursor_line == 19);
    }
    SUBCASE("zt/zb pin the edge, cursor still stays") {
        const auto t = apply_scroll(g, ScrollRequest{ScrollId::Top, 19, 0,
                                                     1, false});
        CHECK(t.top == 19);
        CHECK(t.cursor_line == 19);
        const auto b = apply_scroll(g, ScrollRequest{ScrollId::Bottom, 9, 0,
                                                     1, false});
        CHECK(b.top == 0); // 9 - 21 clamps to the first line
        CHECK(b.cursor_line == 9);
    }
    SUBCASE("counted z commands target a 1-based line and move the cursor") {
        const auto t = apply_scroll(g, ScrollRequest{ScrollId::Top, 9, 0,
                                                     25, true});
        CHECK(t.top == 24);
        CHECK(t.cursor_line == 24); // vim cannot leave the cursor off-screen
        const auto z = apply_scroll(g, ScrollRequest{ScrollId::Center, 19, 0,
                                                     3, true});
        CHECK(z.top == 0); // 2 - 10 clamps
        CHECK(z.cursor_line == 2);
        const auto b = apply_scroll(g, ScrollRequest{ScrollId::Bottom, 4, 0,
                                                     20, true});
        CHECK(b.top == 0);
        CHECK(b.cursor_line == 19);
    }
    SUBCASE("counts past EOF clamp to the last line") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::Top, 0, 0, 999,
                                                     true});
        CHECK(r.top == 39);
        CHECK(r.cursor_line == 39);
    }
    SUBCASE("near-EOF views centre against the window, not the text") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::Center, 39, 18,
                                                     1, false});
        CHECK(r.top == 29); // nvim allows '~' lanes below (oracle)
    }
}

TEST_CASE("view: page scrolls clamp at the file edges (P3.6)") {
    const ViewGeometry g{40, 22, 0}; // page = 20, half = 11

    SUBCASE("C-f scrolls a page and puts the cursor on the new top") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::PageDown, 0, 0,
                                                     1, false});
        CHECK(r.top == 20);
        CHECK(r.cursor_line == 20);
    }
    SUBCASE("C-f with the last line already visible jumps to EOF") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::PageDown, 39, 18,
                                                     1, false});
        CHECK(r.top == 39);
        CHECK(r.cursor_line == 39);
    }
    SUBCASE("C-f count pages, clamped to the last line") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::PageDown, 0, 0,
                                                     9, true});
        CHECK(r.top == 39);
        CHECK(r.cursor_line == 39);
    }
    SUBCASE("C-b puts the cursor on the new bottom line") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::PageUp, 39, 18,
                                                     1, false});
        CHECK(r.top == 0);
        CHECK(r.cursor_line == 21);
    }
    SUBCASE("C-b at the top is a no-op (cursor untouched)") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::PageUp, 4, 0,
                                                     1, false});
        CHECK(r.top == 0);
        CHECK(r.cursor_line == 4);
    }
}

TEST_CASE("view: half-page scrolls never scroll past the last full page") {
    const ViewGeometry g{40, 22, 0}; // half = 11, max top = 40-22 = 18

    SUBCASE("C-d moves cursor and view together") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::HalfDown, 4, 0,
                                                     1, false});
        CHECK(r.top == 11);
        CHECK(r.cursor_line == 15);
    }
    SUBCASE("C-d near EOF clamps the cursor to the last line") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::HalfDown, 34, 13,
                                                     1, false});
        CHECK(r.top == 18); // no '~' lanes for C-d
        CHECK(r.cursor_line == 39);
    }
    SUBCASE("C-d with a count scrolls that many lines") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::HalfDown, 0, 0,
                                                     3, true});
        CHECK(r.top == 3);
        CHECK(r.cursor_line == 3);
    }
    SUBCASE("C-u clamps at the top") {
        const auto r = apply_scroll(g, ScrollRequest{ScrollId::HalfUp, 24, 3,
                                                     1, false});
        CHECK(r.top == 0);
        CHECK(r.cursor_line == 13);
    }
    SUBCASE("short buffer: no scrolling, cursor clamps to the last line") {
        const ViewGeometry short_buf{10, 22, 0};
        const auto r = apply_scroll(short_buf,
                                    ScrollRequest{ScrollId::HalfDown, 5, 0, 1,
                                                  false});
        CHECK(r.top == 0);
        CHECK(r.cursor_line == 9);
    }
}

TEST_CASE("view: follow_top honours scrolloff (P3.6)") {
    SUBCASE("scrolloff keeps rows above the cursor") {
        const ViewGeometry g{100, 10, 2};
        // Cursor one row below the top: the view must scroll back.
        CHECK(follow_top(g, 1, 0) == 0);
        CHECK(follow_top(g, 5, 0) == 0);
        // Cursor near the bottom edge pushes the top.
        CHECK(follow_top(g, 9, 0) == 2);
        CHECK(follow_top(g, 50, 45) == 45); // still inside the so margins
        CHECK(follow_top(g, 56, 45) == 49); // pushed past the bottom margin
    }
    SUBCASE("scrolloff is clamped to half the view") {
        const ViewGeometry g{100, 4, 99};
        CHECK(follow_top(g, 5, 0) == 3); // so == 1: 5+1-4+1
    }
    SUBCASE("top is clamped to the last line") {
        const ViewGeometry g{5, 10, 0};
        CHECK(follow_top(g, 4, 100) == 4);
    }
    SUBCASE("a one-row view still follows (no divide/loop)") {
        const ViewGeometry g{5, 1, 0};
        CHECK(follow_top(g, 3, 0) == 3);
        CHECK(visible_rows(g, 3) == 1);
    }
}

TEST_CASE("view: reducer keeps the aim column through scrolls (P3.6)") {
    EditorState st;
    st.buffers[0].set_lines({"abcdef", "ab", "abcdef"});
    st.win().view_rows = 10;
    st.win().cursor = Pos{0, 4};
    st.win().desired_col = 4;
    st.mode = Mode::Normal;

    // L -> the last visible line, column carried (clamped to the shorter
    // line: "ab" has 2 cps -> col 1).
    key(st, Key{KeyKind::Char, U'L'});
    CHECK(st.win().cursor.line == 2);
    CHECK(st.win().cursor.col == 4);

    st.win().cursor = Pos{0, 4};
    st.win().desired_col = 4;
    key(st, Key{KeyKind::Char, U'M'});
    CHECK(st.win().cursor.line == 1);
    CHECK(st.win().cursor.col == 1); // clamped into "ab"
    CHECK(st.win().desired_col == 4); // the aim survives, like j/k

    // A later vertical move recovers the wanted column.
    key(st, Key{KeyKind::Char, U'j'});
    CHECK(st.win().cursor.col == 4);
}

TEST_CASE("view: z-prefix sequences arm and cancel (P3.6)") {
    EditorState st;
    std::vector<std::string> many;
    for (int i = 0; i < 40; ++i) {
        many.push_back("L" + std::to_string(i));
    }
    st.buffers[0].set_lines(std::move(many));
    st.win().view_rows = 22;
    st.win().cursor = Pos{19, 1};
    st.win().desired_col = 1;
    st.mode = Mode::Normal;

    key(st, Key{KeyKind::Char, U'z'});
    CHECK(st.win().z_pending);
    key(st, Key{KeyKind::Char, U't'});
    CHECK_FALSE(st.win().z_pending);
    CHECK(st.win().top_line == 19);

    // An unknown z-command cancels and keeps the count in limbo (like gg).
    st.count_prefix = 3;
    key(st, Key{KeyKind::Char, U'z'});
    key(st, Key{KeyKind::Char, U'x'});
    CHECK_FALSE(st.win().z_pending);
    CHECK(st.count_prefix == 0);

    // 5zz: explicit count = the target LINE, not a repeat.
    key(st, Key{KeyKind::Char, U'5'});
    key(st, Key{KeyKind::Char, U'z'});
    key(st, Key{KeyKind::Char, U'z'});
    CHECK(st.win().cursor.line == 4);
    CHECK(st.win().top_line == 0); // 4 - 10 clamps
}

TEST_CASE("view: Ctrl-f/d/u reach the reducer and move the viewport (P3.6)") {
    EditorState st;
    std::vector<std::string> many;
    for (int i = 0; i < 40; ++i) {
        many.push_back("L" + std::to_string(i));
    }
    st.buffers[0].set_lines(std::move(many));
    st.win().view_rows = 22;
    st.mode = Mode::Normal;

    key(st, Key{KeyKind::CtrlChar, U'd'});
    CHECK(st.win().cursor.line == 11);
    CHECK(st.win().top_line == 11);

    key(st, Key{KeyKind::CtrlChar, U'u'});
    CHECK(st.win().cursor.line == 0);
    CHECK(st.win().top_line == 0);

    key(st, Key{KeyKind::CtrlChar, U'f'});
    CHECK(st.win().cursor.line == 20);
    CHECK(st.win().top_line == 20);

    key(st, Key{KeyKind::CtrlChar, U'b'});
    CHECK(st.win().cursor.line == 21);
    CHECK(st.win().top_line == 0);

    // The terminal PageDown key mirrors C-f.
    key(st, Key{KeyKind::PageDown, 0});
    CHECK(st.win().top_line == 20);
}

// --- P3.7: '%' bracket match + [count]% ------------------------------------

TEST_CASE("motion: differential replay of the P3.7 percent/bracket fixture") {
    const auto path = find_fixture("motion_p3g.fxt");
    REQUIRE_MESSAGE(path.has_value(),
                    "tests/diff/motion_p3g.fxt not found (build dir relative)");
    std::ifstream in(*path);
    REQUIRE(in.good());

    std::vector<OracleCase> cases;
    std::string line;
    while (std::getline(in, line)) {
        if (auto c = parse_line(line)) {
            cases.push_back(*c);
        }
    }
    REQUIRE_MESSAGE(cases.size() >= 40, "fixture parsed too few oracle cases");

    constexpr std::uint32_t kRows = 22;
    for (const OracleCase& c : cases) {
        INFO("oracle case: ", c.name, "  keys: ", c.keys);
        EditorState st;
        st.buffers[0].set_lines(c.lines);
        st.win().cursor = c.start;
        st.win().desired_col = c.start.col;
        st.win().view_rows = static_cast<int>(kRows);
        st.win().top_line =
            c.start.line > kRows - 1 ? c.start.line - (kRows - 1) : 0;
        st.mode = Mode::Normal;

        for (const Key& k : decode_keys(c.keys)) {
            key(st, k);
            app::follow_cursor(st, static_cast<int>(kRows));
        }

        CHECK(st.win().cursor == c.want);
        CHECK(st.buffers[0].lines() == c.final_lines);
    }
}

TEST_CASE("motion: bracket matching errors keep the cursor (P3.7)") {
    SUBCASE("no bracket at or after the cursor on the line") {
        const std::vector<std::string> ls{"plain text", "(x)"};
        const Motion m{MotionId::MatchBracket};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{0, 3}, 1, 3);
        REQUIRE(!r.has_value());
        CHECK(r.error().msg.size() > 0);
    }
    SUBCASE("unmatched opener / closer") {
        const std::vector<std::string> o{"(abc"};
        const Motion m{MotionId::MatchBracket};
        const auto r = m.apply({&o, k_default_tabwidth}, Pos{0, 0}, 1, 0);
        REQUIRE(!r.has_value());
        const std::vector<std::string> c{"abc)"};
        const auto rc = m.apply({&c, k_default_tabwidth}, Pos{0, 3}, 1, 3);
        REQUIRE(!rc.has_value());
    }
    SUBCASE("a mismatched pair is not a match") {
        const std::vector<std::string> ls{"(]"};
        const Motion m{MotionId::MatchBracket};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{0, 0}, 1, 0);
        REQUIRE(!r.has_value());
    }
    SUBCASE("percent over 100 is rejected") {
        const std::vector<std::string> ls{"a", "b", "c"};
        const Motion m{MotionId::PercentLine};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{0, 0}, 101, 0);
        REQUIRE(!r.has_value());
    }
}

TEST_CASE("motion: bracket matching across lines and mixed types (P3.7)") {
    const Motion m{MotionId::MatchBracket};
    SUBCASE("forward match counts only the same type") {
        const std::vector<std::string> ls{"([)]"};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{0, 0}, 1, 0);
        REQUIRE(r.has_value());
        CHECK(*r == Pos{0, 2}); // '(' pairs with ')' at index 2
    }
    SUBCASE("multi-line braces") {
        const std::vector<std::string> ls{"if {", "  (a)", "}"};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{0, 3}, 1, 3);
        REQUIRE(r.has_value());
        CHECK(*r == Pos{2, 0});
        const auto back = m.apply({&ls, k_default_tabwidth}, Pos{2, 0}, 1, 0);
        REQUIRE(back.has_value());
        CHECK(*back == Pos{0, 3});
    }
    SUBCASE("empty lines inside the span are scanned through") {
        const std::vector<std::string> ls{"(", "", ")"};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{0, 0}, 1, 0);
        REQUIRE(r.has_value());
        CHECK(*r == Pos{2, 0});
    }
    SUBCASE("strings are NOT skipped (documented limitation)") {
        const std::vector<std::string> ls{"x = \"(a\"", "y = b)"};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{0, 5}, 1, 5);
        REQUIRE(r.has_value());
        CHECK(*r == Pos{1, 5});
    }
}

TEST_CASE("motion: [count]% percentage jump geometry (P3.7)") {
    std::vector<std::string> p40;
    for (int i = 0; i < 40; ++i) {
        p40.push_back("L" + std::to_string(i));
    }
    const Motion m{MotionId::PercentLine};

    SUBCASE("rounded up: (lines*count+99)/100") {
        CHECK(*m.apply({&p40, k_default_tabwidth}, Pos{0, 0}, 50, 0)
              == Pos{19, 0});
        CHECK(*m.apply({&p40, k_default_tabwidth}, Pos{0, 0}, 33, 0)
              == Pos{13, 0});
        CHECK(*m.apply({&p40, k_default_tabwidth}, Pos{0, 0}, 99, 0)
              == Pos{39, 0});
        CHECK(*m.apply({&p40, k_default_tabwidth}, Pos{0, 0}, 100, 0)
              == Pos{39, 0});
    }
    SUBCASE("the column is kept and clamped into the target line") {
        const std::vector<std::string> ls{"aaaaaaaa", "bb", "cccccccc"};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{0, 7}, 50, 7);
        REQUIRE(r.has_value());
        CHECK(*r == Pos{1, 1}); // "bb" -> last char
    }
    SUBCASE("landing on the same line leaves the cursor untouched") {
        const std::vector<std::string> ls{"aaa", "bbbbb", "cc"};
        const auto r = m.apply({&ls, k_default_tabwidth}, Pos{1, 4}, 50, 4);
        REQUIRE(r.has_value());
        CHECK(*r == Pos{1, 4});
    }
    SUBCASE("small files: 30% of 7 lines rounds to line 3") {
        const std::vector<std::string> seven{"a", "b", "c", "d", "e", "f", "g"};
        const auto r = m.apply({&seven, k_default_tabwidth}, Pos{0, 0}, 30, 0);
        REQUIRE(r.has_value());
        CHECK(r->line == 2);
    }
}

TEST_CASE("motion: '%' through the reducer picks match vs percentage (P3.7)") {
    EditorState st;
    std::vector<std::string> many;
    for (int i = 0; i < 40; ++i) {
        many.push_back(i == 4 ? "if (a) {" : "L" + std::to_string(i));
    }
    st.buffers[0].set_lines(std::move(many));
    st.win().view_rows = 22;
    st.win().cursor = Pos{4, 3};
    st.win().desired_col = 3;
    st.mode = Mode::Normal;

    // Plain '%' matches the '(' under the cursor (col 3 -> its ')' at col 5).
    key(st, Key{KeyKind::Char, U'%'});
    CHECK(st.win().cursor == Pos{4, 5});

    // With a count it is a percentage jump (keeps the aim column).
    st.win().cursor = Pos{0, 2};
    st.win().desired_col = 2;
    key(st, Key{KeyKind::Char, U'5'});
    key(st, Key{KeyKind::Char, U'0'});
    key(st, Key{KeyKind::Char, U'%'});
    CHECK(st.win().cursor == Pos{19, 2});
    CHECK(st.win().desired_col == 2);

    // An out-of-range percentage reports and does not move.
    st.win().cursor = Pos{19, 2};
    st.msg.clear();
    key(st, Key{KeyKind::Char, U'1'});
    key(st, Key{KeyKind::Char, U'0'});
    key(st, Key{KeyKind::Char, U'1'});
    key(st, Key{KeyKind::Char, U'%'});
    CHECK(st.win().cursor == Pos{19, 2});
    CHECK(!st.msg.empty());
}
