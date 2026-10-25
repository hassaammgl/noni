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

using namespace noni;

namespace {

struct OracleCase {
    std::string name;
    std::string keys;
    Pos start{};
    Pos want{};
    std::vector<std::string> lines;       // starting content (init picture)
    std::vector<std::string> final_lines; // expected end content
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

// Keys replay: '<Esc>' becomes KeyKind::Esc; any other byte is a Char.
std::vector<Key> decode_keys(const std::string& s) {
    std::vector<Key> keys;
    for (std::size_t i = 0; i < s.size();) {
        if (s.compare(i, 5, "<Esc>") == 0) {
            keys.push_back(Key{KeyKind::Esc, 0});
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
    REQUIRE_MESSAGE(cases.size() >= 30, "fixture parsed too few oracle cases");

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
