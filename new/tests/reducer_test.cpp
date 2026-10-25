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
    const Command j = app::command_for_key(st, ch(U'j'));
    CHECK(j.dy == 1);
    const Command dollar = app::command_for_key(st, ch(U'$'));
    CHECK(dollar.dx > 0x10000000);
    const Command zero = app::command_for_key(st, ch(U'0'));
    CHECK(zero.dx < -0x10000000);
    CHECK(app::command_for_key(st, Key{KeyKind::CtrlChar, U'r'}).kind
          == Command::Kind::Redo);
    CHECK(app::command_for_key(st, Key{KeyKind::ArrowLeft, 0}).kind
          == Command::Kind::Motion);
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
    // Enter/Tab land with P2.6 (v0: no command).
    CHECK(app::command_for_key(st, Key{KeyKind::Enter, 0}).kind
          == Command::Kind::None);
    CHECK(app::command_for_key(st, Key{KeyKind::Tab, 0}).kind
          == Command::Kind::None);
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

    key(st, Key{KeyKind::Char, U'$'});
    CHECK(st.win().cursor == Pos{0, 3});
    key(st, Key{KeyKind::Char, U'i'});
    key(st, Key{KeyKind::Char, U' '});
    key(st, Key{KeyKind::Char, U'='});
    key(st, Key{KeyKind::Char, U'='});
    key(st, Key{KeyKind::Char, U' '});
    key(st, Key{KeyKind::Char, U'2'});
    key(st, Key{KeyKind::Char, U'9'});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"ame == 29"});
    key(st, Key{KeyKind::Esc, 0});
    CHECK(st.win().cursor == Pos{0, 8});
    CHECK(st.mode == Mode::Normal);

    key(st, Key{KeyKind::Char, U'j'}); // single line: stays
    key(st, Key{KeyKind::Char, U'k'});
    key(st, Key{KeyKind::Char, U'h'});
    key(st, Key{KeyKind::Char, U'h'});
    key(st, Key{KeyKind::Char, U'h'});
    CHECK(st.win().cursor == Pos{0, 5});
    key(st, Key{KeyKind::ArrowRight, 0});
    key(st, Key{KeyKind::ArrowLeft, 0});
    key(st, Key{KeyKind::ArrowUp, 0});
    key(st, Key{KeyKind::ArrowDown, 0});
    CHECK(st.win().cursor == Pos{0, 5});

    key(st, Key{KeyKind::Char, U'i'});
    key(st, Key{KeyKind::Char, 0xE9}); // é
    key(st, Key{KeyKind::Char, U'!'});
    key(st, Key{KeyKind::Esc, 0});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"ame =é!= 29"});
    CHECK(st.win().cursor == Pos{0, 6});

    key(st, Key{KeyKind::Char, U'0'});
    key(st, Key{KeyKind::ArrowDown, 0}); // single line again
    key(st, Key{KeyKind::Delete, 0});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"me =é!= 29"});
    key(st, Key{KeyKind::Char, U'u'});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"ame =é!= 29"});
    CHECK(st.win().cursor == Pos{0, 0});

    key(st, Key{KeyKind::Char, U'0'});
    key(st, Key{KeyKind::Char, U'x'});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"me =é!= 29"});
    key(st, Key{KeyKind::Char, U'l'});
    key(st, Key{KeyKind::Char, U'u'}); // key 40
    CHECK(st.win().cursor == Pos{0, 0});
    CHECK(st.buffers[0].lines()
          == std::vector<std::string>{"ame =é!= 29"});
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
