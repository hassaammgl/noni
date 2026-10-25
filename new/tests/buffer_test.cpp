#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "core/buffer.hpp"

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
