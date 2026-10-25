#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "app/reducer.hpp"
#include "core/editor_state.hpp"
#include "core/pos.hpp"

using namespace noni;

namespace {

// One scripted keystroke: translate then reduce (the P2.5 loop body).
void key(EditorState& st, const Key& k) {
    noni::app::reducer(st, noni::app::command_for_key(st, k));
}

Key ch(char32_t cp) { return Key{KeyKind::Char, cp}; }

} // namespace

TEST_CASE("reducer: mode-aware dispatch table") {
    EditorState st;

    // Normal mode mappings.
    CHECK(app::command_for_key(st, ch(U'x')).kind == Command::Kind::DeleteChar);
    CHECK(app::command_for_key(st, ch(U'u')).kind == Command::Kind::Undo);
    CHECK(app::command_for_key(st, ch(U'i')).kind
          == Command::Kind::EnterInsert);
    CHECK(app::command_for_key(st, ch(U'A')).kind
          == Command::Kind::EnterInsert);
    CHECK(app::command_for_key(st, ch(U'h')).kind == Command::Kind::Motion);
    CHECK(app::command_for_key(st, ch(U'h')).motion == MotionId::H);
    CHECK(app::command_for_key(st, ch(U'j')).motion == MotionId::J);
    CHECK(app::command_for_key(st, ch(U'k')).motion == MotionId::K);
    CHECK(app::command_for_key(st, ch(U'l')).motion == MotionId::L);
    CHECK(app::command_for_key(st, ch(U'$')).motion == MotionId::Dollar);
    CHECK(app::command_for_key(st, ch(U'w')).motion == MotionId::Word);
    CHECK(app::command_for_key(st, ch(U'W')).motion == MotionId::WordUpper);
    CHECK(app::command_for_key(st, ch(U'b')).motion == MotionId::WordBack);
    CHECK(app::command_for_key(st, ch(U'B')).motion == MotionId::WordBackUpper);
    CHECK(app::command_for_key(st, ch(U'e')).motion == MotionId::WordEnd);
    CHECK(app::command_for_key(st, ch(U'E')).motion == MotionId::WordEndUpper);
    // 'A' enters insert at the append position (at_eol flavor).
    CHECK(app::command_for_key(st, ch(U'A')).at_eol);
    CHECK(!app::command_for_key(st, ch(U'i')).at_eol);
    CHECK(app::command_for_key(st, Key{KeyKind::ArrowLeft, 0}).kind
          == Command::Kind::Motion);
    CHECK(app::command_for_key(st, Key{KeyKind::ArrowLeft, 0}).motion
          == MotionId::H);
    CHECK(app::command_for_key(st, Key{KeyKind::CtrlChar, U'r'}).kind
          == Command::Kind::Redo);
    // Count digits dispatch; '0' after pending digits extends the count.
    CHECK(app::command_for_key(st, ch(U'5')).kind == Command::Kind::CountDigit);
    st.count_prefix = 3;
    CHECK(app::command_for_key(st, ch(U'0')).kind
          == Command::Kind::CountDigit);
    st.count_prefix = 0;
    CHECK(app::command_for_key(st, ch(U'0')).kind == Command::Kind::Motion);
    CHECK(app::command_for_key(st, ch(U'0')).motion == MotionId::Zero);
    // Plain Esc in normal mode is a no-op.
    CHECK(app::command_for_key(st, Key{KeyKind::Esc, 0}).kind
          == Command::Kind::None);
    // Unmapped char.
    CHECK(app::command_for_key(st, ch(U'Z')).kind == Command::Kind::None);

    // Insert mode mappings.
    st.mode = Mode::Insert;
    CHECK(app::command_for_key(st, ch(U'x')).kind
          == Command::Kind::InsertChar);
    CHECK(app::command_for_key(st, ch(U'x')).cp == U'x');
    CHECK(app::command_for_key(st, Key{KeyKind::Backspace, 0}).kind
          == Command::Kind::EraseChar);
    CHECK(app::command_for_key(st, Key{KeyKind::Esc, 0}).kind
          == Command::Kind::LeaveInsert);
    // P2.6: Enter is the indentation-aware split; Tab is a literal '\t'.
    CHECK(app::command_for_key(st, Key{KeyKind::Enter, 0}).kind
          == Command::Kind::InsertNewline);
    const Command tab = app::command_for_key(st, Key{KeyKind::Tab, 0});
    CHECK(tab.kind
          == Command::Kind::InsertChar);
    CHECK(tab.cp == U'\t');
    CHECK(app::command_for_key(st, Key{KeyKind::PasteBegin, 0}).kind
          == Command::Kind::PasteBegin);
    CHECK(app::command_for_key(st, Key{KeyKind::PasteEnd, 0}).kind
          == Command::Kind::PasteEnd);
    // Plain '\n' as Char data splits without indent only while pasting.
    CHECK(app::command_for_key(st, Key{KeyKind::Char, U'\n'}).kind
          == Command::Kind::SplitLine);
}

TEST_CASE("reducer: 40-key scripted session ends in asserted state") {
    EditorState st; // empty buffer, one window, normal mode

    // 1..5: i n a m e        -> "name", cursor (0,4) in insert
    // 6: Esc                 -> Normal, cursor (0,3) (vim: one col back)
    // 7..8: 0 x              -> erase 'n' -> "ame", cursor (0,0)
    // 9: u                   -> undo back to "name", cursor (0,0)
    // 10: Ctrl+r             -> redo -> "ame", cursor (0,0)
    // 11..12: $ i            -> jump to col 3, insert
    // 13..18: " == 2 9"... " == 29" typed -> "ame == 29", cursor (0,9)
    // 19: Esc                -> Normal, cursor (0,8)
    // 20..24: j k h h h      -> line stays 0 (single line), col 8->5
    // 25..28: arrow R L U D  -> col 6,5,5,5
    // 29..31: i é !          -> "ame =é!= 29", cursor (0,7) in insert
    // 32: Esc                -> cursor (0,6)
    // 33..34: 0 j            -> cursor (0,0)
    // 35: Del (Delete key)   -> erase 'a' -> "me =é!= 29", cursor (0,0)
    // 36: u                  -> undo -> "ame =é!= 29", cursor (0,0)
    // 37: 0                  -> cursor (0,0)
    // 38: x                  -> erase 'a' again -> "me =é!= 29", cursor (0,0)
    // 39: l                  -> cursor (0,1)
    // 40: u                  -> undo -> "ame =é!= 29", cursor (0,0)
    key(st, Key{KeyKind::Char, U'i'});
    key(st, Key{KeyKind::Char, U'n'});
    key(st, Key{KeyKind::Char, U'a'});
    key(st, Key{KeyKind::Char, U'm'});
    key(st, Key{KeyKind::Char, U'e'});
    CHECK(st.mode == Mode::Insert);
    CHECK(st.win().cursor == Pos{0, 4});

    key(st, Key{KeyKind::Esc, 0});
    CHECK(st.mode == Mode::Normal);
    CHECK(st.win().cursor == Pos{0, 3});

    key(st, Key{KeyKind::Char, U'0'});
    CHECK(st.win().cursor == Pos{0, 0});
    key(st, Key{KeyKind::Char, U'x'});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"ame"});
    key(st, Key{KeyKind::Char, U'u'});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"name"});
    key(st, Key{KeyKind::CtrlChar, U'r'});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"ame"});

    key(st, Key{KeyKind::Char, U'$'}); // Dollar = ON the last char now
    CHECK(st.win().cursor == Pos{0, 2});
    key(st, Key{KeyKind::Char, U'i'});
    key(st, Key{KeyKind::Char, U' '});
    key(st, Key{KeyKind::Char, U'='});
    key(st, Key{KeyKind::Char, U'='});
    key(st, Key{KeyKind::Char, U' '});
    key(st, Key{KeyKind::Char, U'2'});
    key(st, Key{KeyKind::Char, U'9'});
    // Insert ran at col 2, pushing the 'e' right (vim mid-line insert).
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"am == 29e"});
    key(st, Key{KeyKind::Esc, 0});
    CHECK(st.win().cursor == Pos{0, 7});
    CHECK(st.mode == Mode::Normal);

    key(st, Key{KeyKind::Char, U'j'}); // single line: stays, col clamps len-1
    key(st, Key{KeyKind::Char, U'k'});
    key(st, Key{KeyKind::Char, U'h'});
    key(st, Key{KeyKind::Char, U'h'});
    key(st, Key{KeyKind::Char, U'h'});
    CHECK(st.win().cursor == Pos{0, 4});
    key(st, Key{KeyKind::ArrowRight, 0});
    key(st, Key{KeyKind::ArrowLeft, 0});
    key(st, Key{KeyKind::ArrowUp, 0});
    key(st, Key{KeyKind::ArrowDown, 0});
    CHECK(st.win().cursor == Pos{0, 4});

    key(st, Key{KeyKind::Char, U'i'});
    key(st, Key{KeyKind::Char, 0xE9}); // é
    key(st, Key{KeyKind::Char, U'!'});
    key(st, Key{KeyKind::Esc, 0});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"am =é!= 29e"});
    CHECK(st.win().cursor == Pos{0, 5});

    key(st, Key{KeyKind::Char, U'0'});
    key(st, Key{KeyKind::ArrowDown, 0}); // single line again
    key(st, Key{KeyKind::Delete, 0});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"m =é!= 29e"});
    key(st, Key{KeyKind::Char, U'u'});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"am =é!= 29e"});
    CHECK(st.win().cursor == Pos{0, 0});

    key(st, Key{KeyKind::Char, U'0'});
    key(st, Key{KeyKind::Char, U'x'});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"m =é!= 29e"});
    key(st, Key{KeyKind::Char, U'l'});
    key(st, Key{KeyKind::Char, U'u'}); // key 40
    CHECK(st.win().cursor == Pos{0, 0});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"am =é!= 29e"});
    CHECK(st.mode == Mode::Normal);
    CHECK(st.msg.empty());
    REQUIRE(st.buffers[0].undo_depth() > 0);
}

TEST_CASE("reducer: undo with empty history fills msg") {
    EditorState st;
    key(st, Key{KeyKind::Char, U'u'});
    CHECK(!st.msg.empty());
    key(st, Key{KeyKind::CtrlChar, U'r'});
    CHECK(!st.msg.empty());
}

TEST_CASE("reducer: invalid BufferId no-ops safely") {
    EditorState st;
    st.win().buffer = k_no_buffer;      // sentinel id
    st.win().cursor = Pos{7, 5};        // must stay untouched
    const Pos before{7, 5};

    key(st, ch(U'x'));                  // DeleteChar
    key(st, ch(U'i'));                  // EnterInsert touches mode only
    key(st, ch(U'w'));                  // InsertChar on invalid id
    key(st, Key{KeyKind::Backspace, 0});// EraseChar
    key(st, Key{KeyKind::ArrowRight, 0});// Motion
    key(st, Key{KeyKind::Char, U'u'});  // Undo
    key(st, Key{KeyKind::CtrlChar, U'r'});

    // Mode/marks may change; cursor and buffers must not.
    CHECK(st.win().cursor == before);
    CHECK(st.buffers.size() == 1);
    CHECK(st.buffers[0].lines() == std::vector<std::string>{""});
    CHECK(!st.buffers[0].dirty());
}

TEST_CASE("reducer: insert run merges into one undo unit") {
    EditorState st; // buffer 0, empty
    st.mode = Mode::Insert;
    key(st, Key{KeyKind::Char, U'a'});
    key(st, Key{KeyKind::Char, U'b'});
    key(st, Key{KeyKind::Char, U'c'});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"abc"});
    st.mode = Mode::Normal;
    key(st, Key{KeyKind::Char, U'u'});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{""});
    CHECK(st.win().cursor == Pos{0, 0}); // vim: cursor to change start
    key(st, Key{KeyKind::CtrlChar, U'r'});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"abc"});
}

TEST_CASE("reducer: follow_cursor keeps the cursor line visible") {
    SUBCASE("cursor below the viewport scrolls down") {
        EditorState st;
        std::vector<std::string> many;
        for (int i = 0; i < 30; ++i) {
            many.push_back(std::to_string(i));
        }
        st.buffers[0].set_lines(std::move(many));
        st.win().cursor = Pos{9, 0};
        app::follow_cursor(st, 4); // visible rows 0..3
        CHECK(st.win().top_line == 6);
    }
    SUBCASE("cursor above the top scrolls up") {
        EditorState st;
        st.win().top_line = 5;
        st.win().cursor = Pos{2, 0};
        app::follow_cursor(st, 10);
        CHECK(st.win().top_line == 2);
    }
    SUBCASE("cursor at bottom edge scrolls by rows-1") {
        EditorState st;
        st.win().top_line = 0;
        st.win().cursor = Pos{7, 0};
        app::follow_cursor(st, 4); // visible rows 0..3; 7 outside
        CHECK(st.win().top_line == 4); // 7 - (4-1)
    }
    SUBCASE("cursor on the last visible row does not scroll") {
        EditorState st;
        st.win().top_line = 2;
        st.win().cursor = Pos{5, 0};
        app::follow_cursor(st, 4); // last row = 2+4-1 = 5
        CHECK(st.win().top_line == 2);
    }
    SUBCASE("cursor inside stays untouched") {
        EditorState st;
        st.win().top_line = 3;
        st.win().cursor = Pos{4, 0};
        app::follow_cursor(st, 4);
        CHECK(st.win().top_line == 3);
    }
    SUBCASE("zero/negative view height no-ops") {
        EditorState st;
        st.win().top_line = 0;
        st.win().cursor = Pos{9, 0};
        app::follow_cursor(st, 0);
        CHECK(st.win().top_line == 0);
    }
    SUBCASE("resize_clamp re-enters the buffer grid") {
        std::vector<std::string> many;
        for (int i = 0; i < 30; ++i) {
            many.push_back(std::to_string(i));
        }
        EditorState st;
        st.buffers[0].set_lines(std::move(many));
        st.win().top_line = 20;
        st.win().cursor = Pos{100, 99}; // out of the buffer
        app::resize_clamp(st, 5);
        CHECK(st.win().cursor == Pos{29, 2}); // "29" -> len 2; col clamp
        CHECK(st.win().top_line == 25);       // 29 - (5-1)
    }
}

TEST_CASE("reducer: P2.6 enter copies indent and the run is ONE undo") {
    EditorState st;
    st.buffers[0].set_lines({"  foo"});
    st.win().cursor = Pos{0, 2};
    st.mode = Mode::Insert;

    key(st, Key{KeyKind::Enter, 0});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"  ", "  foo"});
    CHECK(st.win().cursor == Pos{1, 2}); // lands on the copied indent

    key(st, ch(U'x'));
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"  ", "  xfoo"});
    CHECK(st.buffers[0].undo_depth() == 1); // Enter + 'x' = one run

    key(st, Key{KeyKind::Esc, 0});
    CHECK(st.mode == Mode::Normal);
    CHECK(st.win().cursor == Pos{1, 2});

    // nvim oracle: `i...<CR>...<Esc>u` reverts the WHOLE typing stream.
    key(st, ch(U'u'));
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"  foo"});
    CHECK(st.win().cursor == Pos{0, 2});
    CHECK(st.buffers[0].undo_depth() == 0);

    key(st, Key{KeyKind::CtrlChar, U'r'});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"  ", "  xfoo"});
}

TEST_CASE("reducer: P2.6 backspace joins lines and keeps merging") {
    EditorState st;
    st.buffers[0].set_lines({"ab", "cd"});
    st.win().cursor = Pos{1, 0};
    st.mode = Mode::Insert;

    key(st, Key{KeyKind::Backspace, 0});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"abcd"});
    CHECK(st.win().cursor == Pos{0, 2});

    key(st, Key{KeyKind::Backspace, 0}); // erases 'b'
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"acd"});
    key(st, Key{KeyKind::Backspace, 0}); // erases 'a'
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"cd"});
    key(st, Key{KeyKind::Backspace, 0}); // col 0, line 0: no-op
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"cd"});
    CHECK(st.buffers[0].undo_depth() == 1); // all backspaces = one run

    key(st, Key{KeyKind::Esc, 0});
    key(st, ch(U'u')); // one undo restores every backspace incl. the join
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"ab", "cd"});
    CHECK(st.win().cursor == Pos{1, 0});
    CHECK(st.buffers[0].undo_depth() == 0);
}

TEST_CASE("reducer: P2.6 bracketed paste inserts as data") {
    EditorState st; // empty buffer
    st.mode = Mode::Insert;

    // Feeder: markers + raw data (the decoder's PasteBegin/data/PasteEnd).
    key(st, Key{KeyKind::PasteBegin, 0});
    key(st, ch(U'h'));
    key(st, ch(U'i'));
    key(st, Key{KeyKind::Char, U'\n'}); // paste data: split, NO indent copy
    key(st, ch(U'e'));
    key(st, ch(U'd'));
    key(st, Key{KeyKind::PasteEnd, 0});
    CHECK(st.pasting == false);

    // The pasted stream (incl. the \n split) merges into ONE unit.
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"hi", "ed"});
    CHECK(st.win().cursor == Pos{1, 2});
    CHECK(st.buffers[0].undo_depth() == 1);

    key(st, Key{KeyKind::Esc, 0});
    key(st, ch(U'u')); // the pasted stream is one undo unit
    CHECK(st.buffers[0].lines() == std::vector<std::string>{""});
    CHECK(st.win().cursor == Pos{0, 0});
    CHECK(st.buffers[0].undo_depth() == 0);
}

TEST_CASE("reducer: P2.6 plain enter on an empty buffer") {
    EditorState st;
    st.mode = Mode::Insert;
    key(st, Key{KeyKind::Enter, 0});
    CHECK(st.buffers[0].lines() == std::vector<std::string>{"", ""});
    CHECK(st.win().cursor == Pos{1, 0});
    key(st, Key{KeyKind::Esc, 0});
    key(st, ch(U'u'));
    CHECK(st.buffers[0].lines() == std::vector<std::string>{""});
    CHECK(st.win().cursor == Pos{0, 0});
}
