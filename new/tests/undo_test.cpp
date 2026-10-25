#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

#include <string>

#include "core/buffer.hpp"

using namespace noni;

TEST_CASE("undo: single-op undo/redo round-trip") {
    Buffer b{{"hello"}};
    (void)b.insert_char({0, 5}, U'!');
    CHECK(b.lines()[0] == "hello!");

    const auto ok = b.undo();
    REQUIRE(ok.has_value());
    CHECK(b.lines()[0] == "hello");
    // Typing-run single insert: cursor back to the pre-edit hint.
    CHECK(ok->cursor == Pos{0, 5});

    const auto ok1 = b.redo();
    REQUIRE(ok1.has_value());
    CHECK(b.lines()[0] == "hello!");
    CHECK(ok1->cursor == Pos{0, 6});
}

TEST_CASE("undo: nothing to undo/redo is an error, not a crash") {
    Buffer b{{"x"}};
    const auto u = b.undo();
    REQUIRE(!u.has_value());
    const auto r = b.redo();
    REQUIRE(!r.has_value());
}

TEST_CASE("undo: interleaved undo/redo") {
    Buffer b{{"a"}};
    (void)b.insert_char({0, 1}, U'b');
    (void)b.insert_char({0, 2}, U'c');
    CHECK(b.lines()[0] == "abc");

    CHECK(b.undo().has_value());
    CHECK(b.lines()[0] == "ab");
    (void)b.insert_char({0, 2}, U'x');
    CHECK(b.lines()[0] == "abx");
    // Redo is now a dead branch: fresh edit must clear it.
    CHECK(!b.redo().has_value());

    CHECK(b.undo().has_value());
    CHECK(b.lines()[0] == "ab");
    CHECK(b.undo().has_value());
    CHECK(b.lines()[0] == "a");
    CHECK(!b.undo().has_value()); // history floor

    CHECK(b.redo().has_value());
    CHECK(b.lines()[0] == "ab");
    CHECK(b.redo().has_value());
    CHECK(b.lines()[0] == "abx");
    CHECK(!b.redo().has_value()); // ceiling
}

TEST_CASE("undo: erase_range round-trips (dw-shaped edit)") {
    Buffer b{{"foo bar baz", "second"}};
    (void)b.erase_range({{0, 3}, {0, 8}}); // "foo bar baz" -> "foobaz"
    CHECK(b.lines()[0] == "foobaz");

    const auto u = b.undo();
    REQUIRE(u.has_value());
    CHECK(b.lines()[0] == "foo bar baz");
    CHECK(u->cursor == Pos{0, 3}); // cur_before hint restored
}

TEST_CASE("undo: redo empty-buffer floor keeps the >= 1 line invariant") {
    Buffer b{{"aa", "bb"}};
    (void)b.erase_range({{0, 0}, {1, 2}});
    CHECK(b.lines() == std::vector<std::string>{""});

    const auto u = b.undo();
    REQUIRE(u.has_value());
    CHECK(b.lines() == std::vector<std::string>{"aa", "bb"});
}

TEST_CASE("undo: grouped ops become ONE unit") {
    Buffer b{{"aa", "bb", "cc"}};
    b.begin_undo_group();
    (void)b.erase_range({{1, 0}, {2, 2}}); // clears lines 1..2 to one empty
    CHECK(b.lines() == std::vector<std::string>{"aa", ""});
    (void)b.insert_char({1, 0}, U'X'); // typing inside the same op
    b.end_undo_group();
    CHECK(b.undo_depth() == 1);
    CHECK(b.lines() == std::vector<std::string>{"aa", "X"});

    const auto u = b.undo();
    REQUIRE(u.has_value());
    CHECK(b.lines() == std::vector<std::string>{"aa", "bb", "cc"}); // both back
    CHECK(u->cursor == Pos{1, 0});
}

TEST_CASE("undo: branch truncation clears redo") {
    Buffer b{{"a", "b", "c", "d"}};
    (void)b.erase_range({{1, 0}, {2, 1}}); // 4 lines -> 3
    CHECK(b.undo_depth() == 1);
    CHECK(b.redo_depth() == 0);

    CHECK(b.undo().has_value());
    CHECK(b.redo_depth() == 1);

    // New edit while redo exists: redo branch must die.
    (void)b.insert_char({0, 0}, U'!');
    CHECK(b.redo_depth() == 0);
    (void)b.redo();
    CHECK(!b.redo().has_value());
}

TEST_CASE("undo: cap 200 evicts the oldest") {
    Buffer b{{""}};
    for (int i = 0; i < 210; ++i) {
        (void)b.insert_char({0, 0}, static_cast<char32_t>(U'0' + (i % 10)));
    }
    CHECK(b.undo_depth() == UndoStack::k_cap); // 200, not 210

    for (std::size_t i = 0; i < UndoStack::k_cap; ++i) {
        CHECK(b.undo().has_value());
    }
    CHECK(!b.undo().has_value()); // exactly k_cap steps available
    // The 10 evicted eldest inserts must NOT be restorable.
    CHECK(b.lines()[0].size() >= 10); // still many chars left
    CHECK(b.redo().has_value());
}

TEST_CASE("undo: nested groups reuse the inner group") {
    Buffer b{{"aa"}};
    b.begin_undo_group();
    b.begin_undo_group();
    (void)b.insert_char({0, 0}, U'x');
    b.end_undo_group(); // inner close: outer still open
    CHECK(b.undo_depth() == 0);
    (void)b.insert_char({0, 3}, U'y'); // "xaa" -> "xaay" (y appended inside group)
    b.end_undo_group();                // outer close commits ONE unit
    CHECK(b.undo_depth() == 1);
    CHECK(b.lines()[0] == "xaay");

    const auto u = b.undo();
    REQUIRE(u.has_value());
    CHECK(b.lines()[0] == "aa"); // one undo restores everything
}
