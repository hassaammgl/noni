#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

#include <string>

#include "core/text.hpp"

using namespace noni;

TEST_CASE("text: utf8 decode basics (1/2/3/4-byte)") {
    SUBCASE("ascii") {
        const auto d = decode_utf8("abc", 1);
        REQUIRE(d.has_value());
        CHECK(d->first == U'b');
        CHECK(d->second == 1);
    }
    SUBCASE("two-byte é") {
        const std::string s = "\xC3\xA9";
        const auto d = decode_utf8(s, 0);
        REQUIRE(d.has_value());
        CHECK(d->first == U'é');
        CHECK(d->second == 2);
    }
    SUBCASE("three-byte €") {
        const std::string s = "\xE2\x82\xAC";
        const auto d = decode_utf8(s, 0);
        REQUIRE(d.has_value());
        CHECK(d->first == U'€');
        CHECK(d->second == 3);
    }
    SUBCASE("four-byte U+1F600") {
        const std::string s = "\xF0\x9F\x98\x80";
        const auto d = decode_utf8(s, 0);
        REQUIRE(d.has_value());
        CHECK(d->first == U'\U0001F600');
        CHECK(d->second == 4);
    }
}

TEST_CASE("text: utf8 decode rejects malformed sequences") {
    SUBCASE("lone continuation byte") {
        const std::string s = "\x80";
        CHECK(!decode_utf8(s, 0).has_value());
    }
    SUBCASE("truncated 2-byte") {
        const std::string s = "\xC3";
        CHECK(!decode_utf8(s, 0).has_value());
    }
    SUBCASE("truncated 4-byte") {
        const std::string s = "\xF0\x9F\x98";
        CHECK(!decode_utf8(s, 0).has_value());
    }
    SUBCASE("overlong encoded 'a' (C1 AF)") {
        const std::string s = "\xC1\xAF";
        CHECK(!decode_utf8(s, 0).has_value());
    }
    SUBCASE("surrogate ED A0 80 (U+D800)") {
        const std::string s = "\xED\xA0\x80";
        CHECK(!decode_utf8(s, 0).has_value());
    }
    SUBCASE("encodes past U+10FFFF (F5 prefix)") {
        const std::string s = "\xF5\x80\x80\x80";
        CHECK(!decode_utf8(s, 0).has_value());
    }
    SUBCASE("offset past end") {
        CHECK(!decode_utf8("ab", 5).has_value());
    }
}

TEST_CASE("text: decode/encode round-trips") {
    SUBCASE("héllo: cp<->byte") {
        const std::string s = "h\xC3\xA9llo"; // h é l l o = 5 cps, 6 bytes
        CHECK(line_len_cp(s) == 5);
        CHECK(cp_to_byte(s, 0) == 0);
        CHECK(cp_to_byte(s, 1) == 1);
        CHECK(cp_to_byte(s, 2) == 3); // 'l' after the 2-byte é
        CHECK(cp_to_byte(s, 5) == 6); // clamped end
        CHECK(cp_to_byte(s, 99) == 6);

        CHECK(byte_to_cp(s, 0) == 0);
        CHECK(byte_to_cp(s, 2) == 1);
        CHECK(byte_to_cp(s, 3) == 2); // byte in the middle of nothing; 'l' start
        CHECK(byte_to_cp(s, 1) == 1); // inside é -> owning cp index
        CHECK(byte_to_cp(s, 99) == 5);
    }
    SUBCASE("4-byte emoji line") {
        const std::string s = "a\xF0\x9F\x98\x80" "b";
        CHECK(line_len_cp(s) == 3);
        CHECK(byte_to_cp(s, 1) == 1);
        CHECK(byte_to_cp(s, 2) == 1); // middle of the emoji
        CHECK(cp_to_byte(s, 1) == 1);
        CHECK(cp_to_byte(s, 2) == 5);
        CHECK(cp_to_byte(s, 3) == 6);
    }
    SUBCASE("encode basic ascii/wide") {
        CHECK(encode_utf8(U'b') == "b");
        CHECK(encode_utf8(U'é') == "\xC3\xA9");
        CHECK(encode_utf8(U'€') == "\xE2\x82\xAC");
        CHECK(encode_utf8(U'\U0001F600') == "\xF0\x9F\x98\x80");
    }
    SUBCASE("round-trip: encode then decode") {
        for (const char32_t cp : {U'A', U'é', U'€', U'世', U'\U0001F600'}) {
            const std::string enc = encode_utf8(cp);
            const auto d = decode_utf8(enc, 0);
            REQUIRE(d.has_value());
            CHECK(d->first == cp);
            CHECK(d->second == enc.size());
        }
    }
}

TEST_CASE("text: malformed line is addressable (lenient)") {
    // One invalid byte should count as one pseudo-codepoint.
    const std::string s = "a\xFF" "b\xC3"; // 4 cps: a, bad, b, bad(truncated)
    CHECK(line_len_cp(s) == 4);
    CHECK(byte_to_cp(s, 1) == 1);
    CHECK(byte_to_cp(s, 2) == 2);
    CHECK(cp_to_byte(s, 2) == 2);
    CHECK(cp_to_byte(s, 4) == 4);
}

TEST_CASE("text: display width (tab math incl cols 3 and 9)") {
    SUBCASE("tabwidth 4") {
        CHECK(cp_display_width(U'\t', 4, 0) == 4); // 0..3 -> col 4
        CHECK(cp_display_width(U'\t', 4, 3) == 1);
        CHECK(cp_display_width(U'\t', 4, 4) == 4);
        CHECK(cp_display_width(U'\t', 4, 9) == 3);
        CHECK(cp_display_width(U'\t', 4, 12) == 4);
    }
    SUBCASE("tabwidth 8 (default-ish)") {
        CHECK(cp_display_width(U'\t', 8, 5) == 3);
        CHECK(cp_display_width(U'\t', 8, 8) == 8);
    }
    SUBCASE("tabwidth 0 falls back to 8") {
        CHECK(cp_display_width(U'\t', 0, 0) == 8);
    }
    SUBCASE("regular cps are 1 wide") {
        CHECK(cp_display_width(U'h', 4, 3) == 1);
        CHECK(cp_display_width(U'é', 4, 0) == 1);
    }
    SUBCASE("east-asian wide cps are 2") {
        CHECK(cp_display_width(U'世', 4, 0) == 2);
        CHECK(cp_display_width(U'가', 4, 0) == 2);   // Hangul syllable
        CHECK(cp_display_width(U'日', 4, 0) == 2);   // CJK unified
    }
}

TEST_CASE("text: char classes") {
    CHECK(classify(U'a') == CharClass::Word);
    CHECK(classify(U'Z') == CharClass::Word);
    CHECK(classify(U'0') == CharClass::Word);
    CHECK(classify(U'_') == CharClass::Word);
    CHECK(classify(U'é') == CharClass::Word);
    CHECK(classify(U'世') == CharClass::Word);
    CHECK(classify(U'!') == CharClass::Punct);
    CHECK(classify(U'.') == CharClass::Punct);
    CHECK(classify(U' ') == CharClass::Whitespace);
    CHECK(classify(U'\t') == CharClass::Whitespace);

    CHECK(is_blank(U' '));
    CHECK(!is_blank(U'.'));

    // same_word_class: whites group with nothing.
    CHECK(same_word_class(U'a', U'b'));
    CHECK(same_word_class(U'!', U'&'));
    CHECK(same_word_class(U'a', U'!') == false); // word vs punct
    CHECK(same_word_class(U'a', U' ') == false); // word vs whitespace
    CHECK(same_word_class(U' ', U'\t') == false);
}
