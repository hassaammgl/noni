#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "ui/input.hpp"

using namespace noni::ui;

namespace {

// get_wch-shaped fixtures over the PURE decoder (no terminal needed).
std::vector<Key> keys(std::initializer_list<Wch> codes) {
    return decode_stream({codes.begin(), codes.end()});
}

Key one(bool is_key, wint_t code) {
    KeyDecoder d;
    return d.feed(is_key, code);
}

} // namespace

TEST_CASE("input: plain chars and utf8 codepoints") {
    CHECK(one(false, U'a') == Key{KeyKind::Char, U'a'});
    CHECK(one(false, U'9') == Key{KeyKind::Char, U'9'});
    CHECK(one(false, 233) == Key{KeyKind::Char, 233}); // é
    CHECK(one(false, 0x1F600) == Key{KeyKind::Char, 0x1F600}); // emoji
}

TEST_CASE("input: ascii specials + ctrl chars") {
    CHECK(one(false, 0x09) == Key{KeyKind::Tab, 0});
    CHECK(one(false, 0x0A) == Key{KeyKind::Enter, 0}); // Enter == Ctrl+J byte
    CHECK(one(false, 0x0D) == Key{KeyKind::Enter, 0}); // CRLF Enter
    CHECK(one(false, 0x7F) == Key{KeyKind::Backspace, 0});
    CHECK(one(false, 0x18) == Key{KeyKind::CtrlChar, U'x'}); // Ctrl+X
    CHECK(one(false, 0x12) == Key{KeyKind::CtrlChar, U'r'}); // Ctrl+R (P2.7)
    CHECK(one(false, 0x08) == Key{KeyKind::CtrlChar, U'h'}); // Ctrl+H ( lhs )
}

TEST_CASE("input: ncurses KEY_* codes map to kinds") {
    CHECK(one(true, KEY_UP) == Key{KeyKind::ArrowUp, 0});
    CHECK(one(true, KEY_DOWN) == Key{KeyKind::ArrowDown, 0});
    CHECK(one(true, KEY_LEFT) == Key{KeyKind::ArrowLeft, 0});
    CHECK(one(true, KEY_RIGHT) == Key{KeyKind::ArrowRight, 0});
    CHECK(one(true, KEY_HOME) == Key{KeyKind::Home, 0});
    CHECK(one(true, KEY_END) == Key{KeyKind::End, 0});
    CHECK(one(true, KEY_PPAGE) == Key{KeyKind::PageUp, 0});
    CHECK(one(true, KEY_NPAGE) == Key{KeyKind::PageDown, 0});
    CHECK(one(true, KEY_DC) == Key{KeyKind::Delete, 0});
    CHECK(one(true, KEY_RESIZE) == Key{KeyKind::Resize, 0});
    CHECK(one(true, KEY_BACKSPACE) == Key{KeyKind::Backspace, 0});
    CHECK(one(true, 0430) == Key{KeyKind::Unknown, 0430}); // unmapped KEY_*
}

TEST_CASE("input: esc alone and as alt-prefix (two keys)") {
    CHECK(one(false, 0x1B) == Key{KeyKind::Esc, 0});

    const auto alt = keys({{false, 0x1B}, {false, U'a'}});
    REQUIRE(alt.size() == 2); // Alt is an ESC prefix, not a mod (documented)
    CHECK(alt[0] == Key{KeyKind::Esc, 0});
    CHECK(alt[1] == Key{KeyKind::Char, U'a'});
}

TEST_CASE("input: bracketed paste decodes as data (live define_key path)") {
    // Live path: Screen::open registered ESC[200~/ESC[201~ with define_key, so
    // ncurses hands us dedicated key codes; between them every byte is DATA.
    const auto ks = keys({{true, k_paste_begin_code},
                          {false, U'h'},
                          {false, U'i'},
                          {false, 0x0A}, // newline INSIDE paste = data, not Enter
                          {false, 0x7F}, // 0x7F = data too, not Backspace
                          {true, k_paste_end_code}});
    REQUIRE(ks.size() == 6);
    CHECK(ks[0] == Key{KeyKind::PasteBegin, 0});
    CHECK(ks[1] == Key{KeyKind::Char, U'h'});
    CHECK(ks[2] == Key{KeyKind::Char, U'i'});
    CHECK(ks[3] == Key{KeyKind::Char, 0x0A});
    CHECK(ks[4] == Key{KeyKind::Char, 0x7F});
    CHECK(ks[5] == Key{KeyKind::PasteEnd, 0});

    SUBCASE("paste with regex-ish text still decodes as data") {
        const auto more =
            keys({{true, k_paste_begin_code},
                  {false, U'\\'}, {false, U'1'}, {false, U'\n'},
                  {true, k_paste_end_code}});
        REQUIRE(more.size() == 5);
        CHECK(more[0] == Key{KeyKind::PasteBegin, 0});
        CHECK(more[1] == Key{KeyKind::Char, U'\\'});
        CHECK(more[2] == Key{KeyKind::Char, U'1'});
        CHECK(more[3] == Key{KeyKind::Char, U'\n'});
        CHECK(more[4] == Key{KeyKind::PasteEnd, 0});
    }

    SUBCASE("fallback byte path (no define_key): stray Esc before begin") {
        // Exotic terminal: ESC [ 200 ~ byte-by-byte. Esc is emitted first
        // (documented limitation), then the marker still arms paste mode.
        const auto fb = keys({{false, 0x1B}, {false, U'['},
                              {false, U'2'}, {false, U'0'}, {false, U'0'},
                              {false, U'~'},
                              {false, U'x'}, {false, 0x0A}});
        REQUIRE(fb.size() == 4);
        CHECK(fb[0] == Key{KeyKind::Esc, 0});
        CHECK(fb[1] == Key{KeyKind::PasteBegin, 0});
        CHECK(fb[2] == Key{KeyKind::Char, U'x'});
        CHECK(fb[3] == Key{KeyKind::Char, 0x0A}); // raw newline
    }
}

TEST_CASE("input: unknown CSI degrades to Esc + data") {
    const auto ks = keys({{false, 0x1B}, {false, U'['},
                          {false, U'9'}, {false, U'9'}, {false, U'~'},
                          {false, U'z'}});
    // Esc + one Unknown marker + '~' and 'z' as chars
    REQUIRE(ks.size() == 3);
    CHECK(ks[0] == Key{KeyKind::Esc, 0});
    CHECK(ks[1].kind == KeyKind::Unknown);
    CHECK(ks[2] == Key{KeyKind::Char, U'z'});
}

TEST_CASE("input: malformed CSI mid-sequence flushes as chars") {
    const auto ks = keys({{false, 0x1B}, {false, U'['},
                          {false, U'2'}, {false, U'0'}, {false, U'a'}});
    // ESC emitted; '[' opened CSI; digits buffered; flush at 'a' -> Char.
    REQUIRE(ks.size() == 2);
    CHECK(ks[0] == Key{KeyKind::Esc, 0});
    CHECK(ks[1] == Key{KeyKind::Char, U'a'});
}

TEST_CASE("input: key_debug renders stable names") {
    CHECK(key_debug(Key{KeyKind::Char, U'a'}) == "Char:61");
    CHECK(key_debug(Key{KeyKind::Esc, 0}) == "Esc");
    CHECK(key_debug(Key{KeyKind::CtrlChar, U'r'}) == "Ctrl+r");
    CHECK(key_debug(Key{KeyKind::ArrowUp, 0}) == "Up");
    CHECK(key_debug(Key{KeyKind::PasteBegin, 0}) == "PasteBegin");
}
