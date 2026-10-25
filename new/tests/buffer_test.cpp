#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "core/buffer.hpp"
#include "core/text.hpp"

using namespace noni;
namespace fs = std::filesystem;

TEST_CASE("buffer: construction keeps the >= 1 line invariant") {
    SUBCASE("empty vector -> one empty line") {
        Buffer b{{}};
        CHECK(b.lines().size() == 1);
        CHECK(b.lines()[0] == "");
    }
    SUBCASE("normal content is kept") {
        Buffer b{{"a", "b"}};
        CHECK(b.lines().size() == 2);
        CHECK(b.lines()[1] == "b");
    }
    SUBCASE("fresh buffer is clean, revision 0, no path") {
        Buffer b{{"x"}};
        CHECK(!b.dirty());
        CHECK(b.revision() == 0);
        CHECK(b.path().empty());
    }
}

TEST_CASE("buffer: split/join file content") {
    SUBCASE("basic split") {
        const auto v = split_file_content("one\ntwo\nthree");
        CHECK(v == std::vector<std::string>{"one", "two", "three"});
    }
    SUBCASE("CRLF is stripped to LF") {
        const auto v = split_file_content("a\r\nb\r\nc\r");
        CHECK(v == std::vector<std::string>{"a", "b", "c"});
    }
    SUBCASE("trailing newline does not add an empty line") {
        const auto v = split_file_content("a\nb\n");
        CHECK(v == std::vector<std::string>{"a", "b"});
    }
    SUBCASE("blank line in the middle survives, file ends with newline") {
        const auto v = split_file_content("a\n\n\nb\n");
        CHECK(v == std::vector<std::string>{"a", "", "", "b"});
    }
    SUBCASE("missing final newline keeps the last line") {
        const auto v = split_file_content("a\nb");
        CHECK(v == std::vector<std::string>{"a", "b"});
    }
    SUBCASE("empty content -> one empty line") {
        const auto v = split_file_content("");
        CHECK(v == std::vector<std::string>{""});
    }
    SUBCASE("join round-trips") {
        CHECK(join_file_content({"a", "b"}) == "a\nb\n");
    }
    SUBCASE("NUL bytes are kept raw") {
        const std::string s = std::string("a\0b\n", 4);
        const auto v = split_file_content(s);
        CHECK(v.size() == 1);
        CHECK(v[0] == std::string("a\0b", 3));
    }
}

TEST_CASE("buffer: dirty/revision after primitive edits") {
    Buffer b{{"hello"}};
    (void)b.insert_char({0, 5}, U'!');
    CHECK(b.dirty());
    CHECK(b.revision() == 1);
}

TEST_CASE("buffer: insert_char") {
    SUBCASE("basic ascii") {
        Buffer b{{"ac", "x"}};
        const Pos r = b.insert_char({0, 1}, U'b');
        CHECK(b.lines()[0] == "abc");
        CHECK(r == Pos{0, 2});
    }
    SUBCASE("unicode cp inserted in the middle") {
        Buffer b{{"héllo"}}; // h é l l o
        const Pos r = b.insert_char({0, 2}, U'★');
        CHECK(line_len_cp(b.lines()[0]) == 6);
        CHECK(byte_to_cp(b.lines()[0], 4) == 2); // ★ begins at byte 4? h=1 é=2 -> byte 3..? checked below
        CHECK(line_len_cp(b.lines()[0]) == 6);
        CHECK(cp_to_byte(b.lines()[0], 2) == 3); // ★ starts after h(1)+é(2)
        CHECK(r == Pos{0, 3});
    }
    SUBCASE("clamp: line out of range appends to last line") {
        Buffer b{{"a"}};
        const Pos r = b.insert_char({5, 0}, U'Z');
        CHECK(b.lines()[0] == "Za");
        CHECK(r == Pos{0, 1});
    }
    SUBCASE("insert at end of line") {
        Buffer b{{"ab"}};
        const Pos r = b.insert_char({0, 2}, U'c');
        CHECK(b.lines()[0] == "abc");
        CHECK(r == Pos{0, 3});
    }
}

TEST_CASE("buffer: insert_text (paste path, multi-line)") {
    SUBCASE("single line suffix") {
        Buffer b{{"ab"}};
        const Pos r = b.insert_text({0, 2}, "cd");
        CHECK(b.lines()[0] == "abcd");
        CHECK(r == Pos{0, 4});
    }
    SUBCASE("multi-line split keeps head and tail") {
        Buffer b{{"abXX", "kept"}};
        const Pos r = b.insert_text({0, 2}, "aa\nbb\ncc");
        CHECK(b.lines() == std::vector<std::string>{"abaa", "bb", "ccXX", "kept"});
        CHECK(r == Pos{2, 2});
    }
    SUBCASE("trailing newline opens an empty line") {
        Buffer b{{"a"}};
        const Pos r = b.insert_text({0, 1}, "x\n");
        CHECK(b.lines() == std::vector<std::string>{"ax", ""});
        CHECK(r == Pos{1, 0});
    }
    SUBCASE("cross-line insert over existing lines") {
        Buffer b{{"A", "B"}};
        (void)b.insert_text({0, 1}, "1\n2\n3\n4");
        // Paste splits at the insertion point; the next line stays below.
        CHECK(b.lines()
              == std::vector<std::string>{"A1", "2", "3", "4", "B"});
    }
}

TEST_CASE("buffer: erase_char") {
    SUBCASE("erases cp at pos, cursor stays") {
        Buffer b{{"abc"}};
        const Pos r = b.erase_char({0, 1});
        CHECK(b.lines()[0] == "ac");
        CHECK(r == Pos{0, 1});
    }
    SUBCASE("erase wide cp") {
        Buffer b{{"a世b"}}; // a 世 b (世 = 3 bytes)
        (void)b.erase_char({0, 1});
        CHECK(b.lines()[0] == "ab");
    }
    SUBCASE("at end of line: no wrap, no change") {
        Buffer b{{"ab", "cd"}};
        const Pos r = b.erase_char({0, 2});
        CHECK(b.lines()[0] == "ab");
        CHECK(b.lines().size() == 2);
        CHECK(r == Pos{0, 2});
    }
    SUBCASE("clamp past end: line clamps, col 0 erases the first cp") {
        Buffer b{{"ab"}};
        (void)b.erase_char({9, 0});
        CHECK(b.lines()[0] == "b");
    }
}

TEST_CASE("buffer: erase_range") {
    SUBCASE("same line middle") {
        Buffer b{{"abcd"}};
        const Pos r = b.erase_range({{0, 1}, {0, 3}});
        CHECK(b.lines()[0] == "ad");
        CHECK(r == Pos{0, 1});
    }
    SUBCASE("whole line to the end") {
        Buffer b{{"abcd"}};
        (void)b.erase_range({{0, 2}, {0, 9}});
        CHECK(b.lines()[0] == "ab");
    }
    SUBCASE("cross-line") {
        Buffer b{{"abXX1", "XX2", "XX3cd"}};
        const Pos r = b.erase_range({{0, 2}, {2, 3}});
        CHECK(b.lines() == std::vector<std::string>{"abcd"});
        CHECK(r == Pos{0, 2});
    }
    SUBCASE("entire buffer leaves one empty line") {
        Buffer b{{"aa", "bb"}};
        (void)b.erase_range({{0, 0}, {1, 2}});
        CHECK(b.lines() == std::vector<std::string>{""});
    }
    SUBCASE("inverted range is normalized") {
        Buffer b{{"abcd"}};
        (void)b.erase_range({{0, 3}, {0, 1}});
        CHECK(b.lines()[0] == "ad");
    }
}

TEST_CASE("buffer: insert_newline") {
    SUBCASE("split middle") {
        Buffer b{{"abcd"}};
        const Pos r = b.insert_newline({0, 2});
        CHECK(b.lines() == std::vector<std::string>{"ab", "cd"});
        CHECK(r == Pos{1, 0});
    }
    SUBCASE("split at start leaves empty line above") {
        Buffer b{{"x"}};
        (void)b.insert_newline({0, 0});
        CHECK(b.lines() == std::vector<std::string>{"", "x"});
    }
    SUBCASE("split at end") {
        Buffer b{{"x"}};
        (void)b.insert_newline({0, 1});
        CHECK(b.lines() == std::vector<std::string>{"x", ""});
    }
}

TEST_CASE("buffer: join") {
    SUBCASE("dumb join concatenates raw lines") {
        Buffer b{{"ab ", "  cd"}};
        const TextRange r = b.join({0, 2}, false);
        CHECK(b.lines()[0] == "ab   cd");
        CHECK(r.start == Pos{0, 3}); // collapsed at the seam
        CHECK(r.start == r.end);
    }
    SUBCASE("smart join collapses side whitespace and adds one space") {
        Buffer b{{"ab\t ", "  cd"}};
        (void)b.join({0, 2}, true);
        CHECK(b.lines()[0] == "ab cd");
    }
    SUBCASE("smart join does not eat internal whitespace of line A") {
        Buffer b{{"ab cd   ", "  ef"}};
        (void)b.join({0, 5}, true);
        CHECK(b.lines()[0] == "ab cd ef");
    }
    SUBCASE("leading whitespace of empty A: no double space") {
        Buffer b{{"", "cd"}};
        (void)b.join({0, 0}, true);
        CHECK(b.lines()[0] == "cd");
    }
    SUBCASE("last line: no-op empty range sentinel") {
        Buffer b{{"aa", "bb"}};
        const TextRange r = b.join({1, 0}, true);
        CHECK(r.start == Pos{1, 0});
        CHECK(r.start == r.end);
        CHECK(b.lines().size() == 2);
    }
}

TEST_CASE("buffer: revisions bump exactly once per primitive") {
    Buffer b{{"aa", "bb"}};
    const std::uint64_t r0 = b.revision();
    (void)b.insert_char({0, 0}, U'x');
    CHECK(b.revision() == r0 + 1);
    (void)b.erase_range({{0, 0}, {1, 1}}); // joins into one line
    CHECK(b.revision() == r0 + 2);
}

TEST_CASE("buffer: record_text_change (reducer path, reserved)") {
    Buffer b{{"abc"}};
    TextChange ch;
    ch.kind = EditKind::Bulk;
    ch.first_line = 0;
    ch.old_lines.push_back("abc");
    ch.new_lines.push_back("xyz");
    ch.cur_before = Pos{0, 0};
    ch.cur_after = Pos{0, 2};
    b.record_text_change(ch);
    CHECK(b.lines()[0] == "xyz");
    CHECK(b.revision() == 1);
}

TEST_CASE("buffer: load") {
    const fs::path dir = fs::temp_directory_path() / "noni_p1_2_load";
    std::error_code ec;
    fs::create_directories(dir, ec);

    SUBCASE("simple file round-trip shape") {
        const fs::path p = dir / "a.txt";
        {
            std::FILE* f = std::fopen(p.c_str(), "wb");
            REQUIRE(f != nullptr);
            std::fwrite("one\ntwo\n", 1, 8, f);
            std::fclose(f);
        }
        auto r = Buffer::load(p);
        REQUIRE(r.has_value());
        CHECK(r->lines() == std::vector<std::string>{"one", "two"});
        CHECK(r->path() == p);
        CHECK(!r->dirty());
    }
    SUBCASE("missing file is an error, never a fallback buffer") {
        const auto r = Buffer::load(dir / "does_not_exist.txt");
        REQUIRE(!r.has_value());
        CHECK(!r.error().msg.empty());
    }
    SUBCASE("empty file loads as one empty line") {
        const fs::path p = dir / "empty.txt";
        {
            std::FILE* f = std::fopen(p.c_str(), "wb");
            REQUIRE(f != nullptr);
            std::fclose(f);
        }
        auto r = Buffer::load(p);
        REQUIRE(r.has_value());
        CHECK(r->lines() == std::vector<std::string>{""});
    }
    SUBCASE("CRLF file loads normalized") {
        const fs::path p = dir / "crlf.txt";
        {
            std::FILE* f = std::fopen(p.c_str(), "wb");
            REQUIRE(f != nullptr);
            std::fwrite("a\r\nb\r\n", 1, 6, f);
            std::fclose(f);
        }
        auto r = Buffer::load(p);
        REQUIRE(r.has_value());
        CHECK(r->lines() == std::vector<std::string>{"a", "b"});
    }

    fs::remove_all(dir, ec);
}

TEST_CASE("buffer: save") {
    const fs::path dir = fs::temp_directory_path() / "noni_p1_2_save";
    std::error_code ec;
    fs::create_directories(dir, ec);

    SUBCASE("save writes LF and round-trips through load") {
        const fs::path p = dir / "out.txt";
        Buffer b{{"héllo", "", "side"}};
        auto w = b.save(p);
        REQUIRE(w.has_value());

        auto r = Buffer::load(p);
        REQUIRE(r.has_value());
        CHECK(r->lines() == b.lines());
        // round-trip bytes: UTF-8 preserved byte-for-byte
        std::FILE* f = std::fopen(p.c_str(), "rb");
        REQUIRE(f != nullptr);
        std::string data;
        char chunk[256];
        std::size_t n = 0;
        while ((n = std::fread(chunk, 1, sizeof chunk, f)) > 0) {
            data.append(chunk, n);
        }
        std::fclose(f);
        CHECK(data == "héllo\n\nside\n");
    }
    SUBCASE("save to an unwritable path is an error") {
        Buffer b{{"x"}};
        const auto w = b.save(dir / "no_dir" / "x.txt");
        REQUIRE(!w.has_value());
        CHECK(!w.error().msg.empty());
    }

    fs::remove_all(dir, ec);
}

TEST_CASE("buffer: set_lines mutates and marks dirty") {
    Buffer b{{"a"}};
    CHECK(b.revision() == 0);
    b.set_lines({{"a", "b", "c"}});
    CHECK(b.dirty());
    CHECK(b.revision() == 1);
    CHECK(b.lines().size() == 3);

    SUBCASE("mark_saved clears dirty without bumping revision") {
        b.mark_saved();
        CHECK(!b.dirty());
        CHECK(b.revision() == 1);
    }
}
