#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "core/buffer_search.hpp"

using namespace noni;

TEST_CASE("search: literal vs regex mode") {
    const std::vector<std::string> lines{"a.c and axc", "nothing here"};

    SUBCASE("literal: the dot is a dot") {
        const auto r = find_all(lines, "a.c", {false, true, false});
        REQUIRE(r.has_value());
        REQUIRE(r->size() == 1);
        CHECK((*r)[0].line == 0);
        CHECK((*r)[0].byte_start == 0);
        CHECK((*r)[0].byte_len == 3);
    }
    SUBCASE("regex: the dot matches any char") {
        const auto r = find_all(lines, "a.c", {true, true, false});
        REQUIRE(r.has_value());
        CHECK(r->size() == 2); // a.c, axc
    }
}

TEST_CASE("search: anchors are per-line") {
    const std::vector<std::string> lines{"foo bar", "x foo", "foofoo"};

    SUBCASE("^foo hits only line starts") {
        const auto r = find_all(lines, "^foo", {true, true, false});
        REQUIRE(r.has_value());
        REQUIRE(r->size() == 2);
        CHECK((*r)[0].line == 0);
        CHECK((*r)[1].line == 2);
    }
    SUBCASE("foo$ hits only line ends") {
        const auto r = find_all(lines, "foo$", {true, true, false});
        REQUIRE(r.has_value());
        REQUIRE(r->size() == 2);
        CHECK((*r)[0].line == 1);
        CHECK((*r)[0].byte_start == 2); // "x foo"
        CHECK((*r)[1].line == 2);
        CHECK((*r)[1].byte_start == 3); // "foofoo"
    }
}

TEST_CASE("search: case folding") {
    const std::vector<std::string> lines{"Hello HELLO hello"};
    auto r = find_all(lines, "hello", {true, false, false}); // case-insensitive
    REQUIRE(r.has_value());
    CHECK(r->size() == 3);
    r = find_all(lines, "hello", {true, true, false}); // case-sensitive
    REQUIRE(r.has_value());
    CHECK(r->size() == 1);
    CHECK((*r)[0].byte_start == 12);
}

TEST_CASE("search: whole_word uses glibc \\b") {
    const std::vector<std::string> lines{"word words swordword word"};
    auto r = find_all(lines, "word", {true, true, true});
    REQUIRE(r.has_value());
    CHECK(r->size() == 2); // first + last occurrences only
    CHECK((*r)[0].byte_start == 0);
    CHECK((*r)[1].byte_start == 21);
}

TEST_CASE("search: per-line byte offsets and multi matches") {
    const std::vector<std::string> lines{"aba", "\xC3\xA9""aba"}; // é + aba
    const auto r = find_all(lines, "ab", {true, true, false});
    REQUIRE(r.has_value());
    REQUIRE(r->size() == 2);
    CHECK((*r)[1].line == 1);
    CHECK((*r)[1].byte_start == 2); // after the 2-byte é
}

TEST_CASE("search: errors") {
    const std::vector<std::string> lines{"x"};
    SUBCASE("empty pattern") {
        CHECK(!find_all(lines, "", {true, true, false}).has_value());
    }
    SUBCASE("unbalanced paren") {
        CHECK(!find_all(lines, "a(", {true, true, false}).has_value());
    }
    SUBCASE("unbalanced bracket") {
        CHECK(!find_all(lines, "a[", {true, true, false}).has_value());
    }
    // replacement shares the same error path
    auto lines_mut = lines;
    CHECK(!replace(lines_mut, "a[", "x", {true, true, false}, true).has_value());
}

TEST_CASE("search: replace basic + refs") {
    SUBCASE("all occurrences") {
        std::vector<std::string> lines{"cat bat cat", "cat"};
        const auto n = replace(lines, "cat", "CAT", {false, true, false}, true);
        REQUIRE(n.has_value());
        CHECK(*n == 3);
        CHECK(lines[0] == "CAT bat CAT");
        CHECK(lines[1] == "CAT");
    }
    SUBCASE("no 'all': first match of the buffer only") {
        std::vector<std::string> lines{"cat bat cat", "cat"};
        const auto n = replace(lines, "cat", "XX", {false, true, false}, false);
        REQUIRE(n.has_value());
        CHECK(*n == 1);
        CHECK(lines[0] == "XX bat cat");
        CHECK(lines[1] == "cat");
    }
    SUBCASE("& whole match") {
        std::vector<std::string> lines{"aaa"};
        CHECK(replace(lines, "a+", "&!", {true, true, false}, true).has_value());
        CHECK(lines[0] == "aaa!");
    }
    SUBCASE("groups via ( )") {
        std::vector<std::string> lines{"hello world"};
        CHECK(replace(lines, "(\\w+) (\\w+)", "\\2, \\1", {true, true, false}, true)
                  .has_value());
        CHECK(lines[0] == "world, hello");
    }
    SUBCASE("escaped \\& is a literal ampersand") {
        std::vector<std::string> lines{"x"};
        CHECK(replace(lines, "x", "\\&", {false, true, false}, true).has_value());
        CHECK(lines[0] == "&");
    }
    SUBCASE("escaped \\\\ is a literal backslash") {
        std::vector<std::string> lines{"x"};
        CHECK(replace(lines, "x", "\\\\", {false, true, false}, true).has_value());
        CHECK(lines[0] == "\\");
    }
    SUBCASE("unset group is empty; set group expands") {
        std::vector<std::string> lines{"ab", "b"};
        CHECK(replace(lines, "(a)?b", "[\\1]", {true, true, false}, true)
                  .has_value());
        CHECK(lines[0] == "[a]");
        CHECK(lines[1] == "[]");
    }
}

TEST_CASE("search: NUL-containing lines still match around the NUL") {
    const std::string nul_line = std::string("ab\0cd", 5);
    const std::vector<std::string> lines{nul_line};
    const auto r = find_all(lines, "cd", {true, true, false});
    REQUIRE(r.has_value());
    REQUIRE(r->size() == 1);
    CHECK((*r)[0].byte_start == 3);
}

TEST_CASE("search: no infinite loop on empty-match patterns") {
    std::vector<std::string> lines{"abcd"};
    const auto n = replace(lines, "e*", "-", {true, true, false}, true);
    REQUIRE(n.has_value());
    CHECK(lines[0] == "-a-b-c-d-"); // trailing zero-width match at EOL included
    CHECK(*n >= 5);
}
