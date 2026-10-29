#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

// P4.1 — operator composition FSM (core/operator.hpp) driven through the REAL
// app path (dispatch + reducer). The FSM only BUILDS an Edit; applying it is
// P4.2+, so these tests assert the composed range/shape and the Pending
// lifecycle, not buffer contents. ACCEPT from ROADMAP: 2d3w == 6 words,
// dd == a line, cc keeps the indent (and enters Insert).
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "app/reducer.hpp"
#include "core/editor_state.hpp"
#include "core/operator.hpp"
#include "core/pos.hpp"

using namespace noni;

namespace {

// One scripted keystroke: translate then reduce (the P2.5 loop body).
void key(EditorState& st, const Key& k) {
    noni::app::reducer(st, noni::app::command_for_key(st, k));
}

// Types `keys`; "<Esc>" is the readable escape token (as in the fixtures).
void press(EditorState& st, const std::string& keys) {
    for (std::size_t i = 0; i < keys.size();) {
        if (keys.compare(i, 5, "<Esc>") == 0) {
            key(st, Key{KeyKind::Esc, 0});
            i += 5;
        } else {
            key(st, Key{KeyKind::Char, static_cast<char32_t>(keys[i])});
            ++i;
        }
    }
}

EditorState fresh(std::vector<std::string> lines, Pos cursor = Pos{0, 0}) {
    EditorState st;
    st.buffers[0].set_lines(std::move(lines));
    st.win().cursor = cursor;
    return st;
}

// The text an Edit covers (ASCII fixtures: byte == codepoint index).
std::string range_text(const std::vector<std::string>& lines,
                       const TextRange& r) {
    std::string out;
    const std::uint32_t last =
        r.end.line < lines.size() ? r.end.line
                                  : static_cast<std::uint32_t>(lines.size());
    for (std::uint32_t line = r.start.line; line <= last; ++line) {
        if (line >= lines.size()) {
            break;
        }
        const std::string& s = lines[line];
        const std::uint32_t from = line == r.start.line ? r.start.col : 0;
        const std::uint32_t to =
            line == r.end.line ? std::min<std::uint32_t>(r.end.col,
                                                         static_cast<std::uint32_t>(s.size()))
                               : static_cast<std::uint32_t>(s.size());
        if (line != r.start.line) {
            out.push_back('\n');
        }
        if (from < to) {
            out.append(s.substr(from, to - from));
        }
    }
    return out;
}

std::uint32_t count_words(const std::string& s, const std::string& seps) {
    std::uint32_t n = 0;
    bool in_word = false;
    for (const char c : s) {
        const bool sep = seps.find(c) != std::string::npos;
        if (!sep && !in_word) {
            ++n;
        }
        in_word = !sep;
    }
    return n;
}

} // namespace

TEST_CASE("operator: [count]op[count]motion composes (ACCEPT 2d3w == 6 words)") {
    EditorState st = fresh({"one two three four five six seven eight nine ten"});

    // Still pending after only the operator: no edit, operator armed.
    press(st, "d");
    CHECK(st.pending_op.phase == Pending::Phase::Op);
    CHECK(!st.last_edit.has_value());

    // A count typed after the operator belongs to the MOTION.
    press(st, "3");
    CHECK(st.pending_op.motion_count == 3);
    CHECK(st.count_prefix == 0);

    key(st, Key{KeyKind::Esc, 0}); // start over
    REQUIRE(st.pending_op.phase == Pending::Phase::Idle);

    press(st, "2d3w");
    REQUIRE(st.last_edit.has_value());
    const Edit& e = *st.last_edit;
    CHECK(e.op == OperatorId::Delete);
    CHECK(e.scope == EditScope::Charwise);
    CHECK(e.range.start == Pos{0, 0});
    CHECK(e.range.end == Pos{0, 28}); // start of the 7th word
    const std::string covered =
        range_text(st.buffers[0].lines(), e.range);
    CHECK(covered == "one two three four five six ");
    CHECK(count_words(covered, " ") == 6); // 2 * 3
    CHECK(st.count_prefix == 0);           // both counts consumed
    CHECK(st.pending_op.phase == Pending::Phase::Idle);
}

TEST_CASE("operator: doubled operator is linewise whole lines (ACCEPT dd)") {
    SUBCASE("dd takes the cursor line") {
        EditorState st = fresh({"aaa", "bbb", "ccc", "ddd"}, Pos{1, 2});
        press(st, "dd");
        REQUIRE(st.last_edit.has_value());
        const Edit& e = *st.last_edit;
        CHECK(e.scope == EditScope::Linewise);
        CHECK(e.lines == 1);
        CHECK(e.range.start == Pos{1, 0});
        CHECK(e.range.end == Pos{2, 0});
        CHECK(!e.enter_insert); // yank/delete do not change mode
    }
    SUBCASE("2dd and d2d and 2d2d multiply") {
        {
            EditorState st = fresh({"aaa", "bbb", "ccc", "ddd", "eee"},
                                   Pos{1, 0});
            press(st, "2dd");
            REQUIRE(st.last_edit.has_value());
            CHECK(st.last_edit->lines == 2);
            CHECK(st.last_edit->range.end == Pos{3, 0});
        }
        {
            EditorState st = fresh({"aaa", "bbb", "ccc", "ddd", "eee"},
                                   Pos{1, 0});
            press(st, "d2d");
            REQUIRE(st.last_edit.has_value());
            CHECK(st.last_edit->lines == 2);
            CHECK(st.last_edit->range.end == Pos{3, 0});
        }
        {
            EditorState st = fresh({"aaa", "bbb", "ccc", "ddd", "eee"},
                                   Pos{1, 0});
            press(st, "2d2d");
            REQUIRE(st.last_edit.has_value());
            CHECK(st.last_edit->lines == 4);
            CHECK(st.last_edit->range.end == Pos{5, 0});
        }
    }
    SUBCASE("yy is linewise yank, no insert") {
        EditorState st = fresh({"aaa", "bbb"}, Pos{0, 1});
        press(st, "yy");
        REQUIRE(st.last_edit.has_value());
        CHECK(st.last_edit->op == OperatorId::Yank);
        CHECK(st.last_edit->scope == EditScope::Linewise);
        CHECK(!st.last_edit->enter_insert);
    }
}

TEST_CASE("operator: cc keeps the indent and enters insert (ACCEPT)") {
    EditorState st = fresh({"    foo", "    bar"}, Pos{0, 6});
    press(st, "cc");
    REQUIRE(st.last_edit.has_value());
    const Edit& e = *st.last_edit;
    CHECK(e.op == OperatorId::Change);
    CHECK(e.scope == EditScope::Linewise);
    CHECK(e.lines == 1);
    CHECK(e.keep_indent);    // P4.3 re-indents from the first affected line
    CHECK(e.enter_insert);   // 'c' leaves the editor in Insert
    CHECK(e.range.start == Pos{0, 0});
    // 'c' with a motion is charwise and also enters Insert (db here).
    EditorState st2 = fresh({"one two three"}, Pos{0, 8});
    press(st2, "cb");
    REQUIRE(st2.last_edit.has_value());
    CHECK(st2.last_edit->scope == EditScope::Charwise);
    CHECK(st2.last_edit->enter_insert);
    CHECK(!st2.last_edit->keep_indent);
}

TEST_CASE("operator: Esc and bad keys cancel without an edit") {
    EditorState st = fresh({"abc def"}, Pos{0, 0});
    press(st, "d");
    REQUIRE(st.pending_op.phase == Pending::Phase::Op);
    press(st, "<Esc>");
    CHECK(st.pending_op.phase == Pending::Phase::Idle);
    CHECK(!st.last_edit.has_value());
    // The arrow keys / Enter are not motions: they abort too (vim beeps).
    press(st, "d");
    key(st, Key{KeyKind::ArrowLeft, 0});
    CHECK(st.pending_op.phase == Pending::Phase::Idle);
    CHECK(!st.last_edit.has_value());
}

TEST_CASE("operator: register prefix is recorded on the edit") {
    EditorState st = fresh({"one two three four five six seven"});
    press(st, "\"a2d3w");
    REQUIRE(st.last_edit.has_value());
    CHECK(st.last_edit->reg == U'a');
    CHECK(count_words(range_text(st.buffers[0].lines(), st.last_edit->range),
                      " ") == 6);
    // The prefix survives as a phase of its own before the operator.
    EditorState st2 = fresh({"abc"});
    press(st2, "\"");
    CHECK(st2.pending_op.phase == Pending::Phase::WantReg);
    press(st2, "b");
    CHECK(st2.pending_op.phase == Pending::Phase::Idle);
    CHECK(st2.pending_op.reg == U'b');
    CHECK(!st2.last_edit.has_value());
}

TEST_CASE("operator: failed and zero-width motions cancel") {
    // dfz: no 'z' on the line -> find fails -> nothing happens (vim beeps).
    EditorState st = fresh({"abc def"}, Pos{0, 0});
    press(st, "dfz");
    CHECK(st.pending_op.phase == Pending::Phase::Idle);
    CHECK(!st.last_edit.has_value());

    // d0 at column 0 covers nothing.
    EditorState st2 = fresh({"abc"}, Pos{0, 0});
    press(st2, "d0");
    CHECK(!st2.last_edit.has_value());
    CHECK(st2.pending_op.phase == Pending::Phase::Idle);

    // 'q' is not a motion: the operator is dropped.
    EditorState st3 = fresh({"abc"}, Pos{0, 0});
    press(st3, "dq");
    CHECK(!st3.last_edit.has_value());
    CHECK(st3.pending_op.phase == Pending::Phase::Idle);
}

TEST_CASE("operator: motion ranges (includes, backward, linewise j/k/G)") {
    SUBCASE("d$ includes the last char: end == len") {
        EditorState st = fresh({"abcd"}, Pos{0, 1});
        press(st, "d$");
        REQUIRE(st.last_edit.has_value());
        CHECK(st.last_edit->range.start == Pos{0, 1});
        CHECK(st.last_edit->range.end == Pos{0, 4});
    }
    SUBCASE("de includes the word end") {
        EditorState st = fresh({"one two"}, Pos{0, 0});
        press(st, "de");
        REQUIRE(st.last_edit.has_value());
        CHECK(range_text(st.buffers[0].lines(), st.last_edit->range) == "one");
    }
    SUBCASE("db is backward and excludes the cursor char") {
        EditorState st = fresh({"one two three"}, Pos{0, 8});
        press(st, "db");
        REQUIRE(st.last_edit.has_value());
        CHECK(st.last_edit->range.start == Pos{0, 4});
        CHECK(st.last_edit->range.end == Pos{0, 8});
        CHECK(range_text(st.buffers[0].lines(), st.last_edit->range) == "two ");
    }
    SUBCASE("dj / dk are linewise (j/k are line motions)") {
        EditorState st = fresh({"aaa", "bbb", "ccc"}, Pos{1, 0});
        press(st, "dj");
        REQUIRE(st.last_edit.has_value());
        CHECK(st.last_edit->scope == EditScope::Linewise);
        CHECK(st.last_edit->lines == 2);
        CHECK(st.last_edit->range.start == Pos{1, 0});
        CHECK(st.last_edit->range.end == Pos{3, 0});

        EditorState st2 = fresh({"aaa", "bbb", "ccc"}, Pos{1, 0});
        press(st2, "dk");
        REQUIRE(st2.last_edit.has_value());
        CHECK(st2.last_edit->lines == 2);
        CHECK(st2.last_edit->range.start == Pos{0, 0});
        CHECK(st2.last_edit->range.end == Pos{2, 0});
    }
    SUBCASE("dG deletes to the last line, inclusive") {
        EditorState st = fresh({"aaa", "bbb", "ccc", "ddd"}, Pos{1, 0});
        press(st, "dG");
        REQUIRE(st.last_edit.has_value());
        CHECK(st.last_edit->scope == EditScope::Linewise);
        CHECK(st.last_edit->lines == 3);
        CHECK(st.last_edit->range.end == Pos{4, 0}); // one past the last line
    }
    SUBCASE("dgg deletes from the first line down to the cursor") {
        EditorState st = fresh({"aaa", "bbb", "ccc", "ddd"}, Pos{2, 1});
        press(st, "dgg");
        REQUIRE(st.last_edit.has_value());
        CHECK(st.last_edit->scope == EditScope::Linewise);
        CHECK(st.last_edit->lines == 3);
        CHECK(st.last_edit->range.start == Pos{0, 0});
        CHECK(st.last_edit->range.end == Pos{3, 0});
    }
}

// --- P4.1 differential: the composed range vs the nvim oracle ----------------
// Fixture (tests/diff/op_p4.fxt, generated once by gen_operator_p4.vim):
//   keys || line,col || regtype || regtext || pict
// nvim's unnamed register holds what the operator covered and getregtype()
// tells the shape ('V' linewise / 'v' charwise); '-' = vim cancelled (beep).
// Our Edit is not applied yet (P4.2), so the test compares the RANGE TEXT the
// Edit describes against the oracle's recorded register text.
namespace {

struct OpCase {
    std::string keys;
    Pos start{};
    char regtype = '-';
    std::string regtext;
    std::vector<std::string> lines;
};

std::vector<std::string> split_pict(const std::string& pict) {
    std::vector<std::string> out;
    std::size_t begin = 0;
    for (;;) {
        const auto at = pict.find('|', begin);
        if (at == std::string::npos) {
            out.push_back(pict.substr(begin));
            return out;
        }
        out.push_back(pict.substr(begin, at - begin));
        begin = at + 1;
    }
}

std::string unescape_nl(const std::string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == 'n') {
            out.push_back('\n');
            ++i;
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

std::optional<OpCase> parse_op_line(const std::string& line) {
    const std::string sep = " || ";
    std::vector<std::string> f;
    std::size_t begin = 0;
    for (;;) {
        const auto at = line.find(sep, begin);
        if (at == std::string::npos) {
            f.push_back(line.substr(begin));
            break;
        }
        f.push_back(line.substr(begin, at - begin));
        begin = at + sep.size();
    }
    if (f.size() != 5 || f[0].empty()) {
        return std::nullopt;
    }
    OpCase c;
    c.keys = f[0];
    const auto comma = f[1].find(',');
    c.start = Pos{static_cast<std::uint32_t>(std::stoul(f[1].substr(0, comma))) - 1,
                  static_cast<std::uint32_t>(std::stoul(f[1].substr(comma + 1))) - 1};
    c.regtype = f[2].empty() ? '-' : f[2][0];
    c.regtext = unescape_nl(f[3]);
    c.lines = split_pict(f[4]);
    return c;
}

// The text an Edit covers, line-separated by '\n' like vim's register.
std::string covered_text(const Edit& e, const std::vector<std::string>& lines) {
    if (e.scope == EditScope::Linewise) {
        std::string out;
        const std::uint32_t last =
            std::min<std::uint32_t>(e.range.end.line,
                                    static_cast<std::uint32_t>(lines.size()));
        for (std::uint32_t line = e.range.start.line; line < last; ++line) {
            if (line != e.range.start.line) {
                out.push_back('\n');
            }
            out.append(lines[line]);
        }
        return out;
    }
    return range_text(lines, e.range);
}

} // namespace

TEST_CASE("operator: differential replay of the P4.1 nvim oracle fixture") {
    const std::string candidates[] = {
        "../tests/diff/op_p4.fxt", "tests/diff/op_p4.fxt",
        "../../tests/diff/op_p4.fxt"};
    std::ifstream in;
    for (const std::string& cand : candidates) {
        in.open(cand);
        if (in.good()) {
            break;
        }
        in.clear();
    }
    REQUIRE_MESSAGE(in.good(), "tests/diff/op_p4.fxt not found");

    std::vector<OpCase> cases;
    std::string line;
    while (std::getline(in, line)) {
        if (auto c = parse_op_line(line)) {
            cases.push_back(std::move(*c));
        }
    }
    REQUIRE(cases.size() > 20); // a truncated fixture must fail loudly
    std::size_t checked = 0;
    for (const OpCase& c : cases) {
        CAPTURE(c.keys);
        CAPTURE(c.start.line);
        CAPTURE(c.start.col);
        EditorState st = fresh(c.lines, c.start);
        press(st, c.keys);
        if (c.regtype == '-') {
            CHECK_FALSE(st.last_edit.has_value()); // oracle cancelled
            CHECK(st.pending_op.phase == Pending::Phase::Idle);
            ++checked;
            continue;
        }
        REQUIRE(st.last_edit.has_value());
        const Edit& e = *st.last_edit;
        CHECK(e.scope == (c.regtype == 'V' ? EditScope::Linewise
                                           : EditScope::Charwise));
        CHECK(covered_text(e, st.buffers[0].lines()) == c.regtext);
        ++checked;
    }
    CHECK(checked == cases.size());
}
