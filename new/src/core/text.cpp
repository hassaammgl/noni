// P1.1 — see text.hpp.
#include "core/text.hpp"

namespace noni {

namespace {

// ASCII fast-path helpers.
constexpr bool ascii_digit(char32_t c) { return c >= U'0' && c <= U'9'; }
constexpr bool ascii_lower(char32_t c) { return c >= U'a' && c <= U'z'; }
constexpr bool ascii_upper(char32_t c) { return c >= U'A' && c <= U'Z'; }

constexpr bool is_whitespace_cp(char32_t c) {
    // Unicode blanks we honor directly (covers the common sources of
    // "whitespace" in text files without needing a locale).
    return c == U' ' || c == U'\t' || c == U'\n' || c == U'\r' || c == U'\v'
        || c == U'\f' || c == U'\u00a0' || c == U'\u2028' || c == U'\u2029'
        || c == U'\u3000'; // ideographic space
}

// Hand-rolled wide ranges (minimal EastAsianWide coverage for rendering;
// precomposed Latin/Greek/Cyrillic etc. stay narrow). Compact, ordered list.
struct WideRange {
    char32_t lo;
    char32_t hi;
};
constexpr WideRange k_wide_ranges[] = {
    {0x1100, 0x115F}, // Hangul Jamo
    {0x2E80, 0x303E}, // CJK Radicals .. CJK Symbols
    {0x3041, 0x33FF}, // Hiragana .. CJK Compatibility
    {0x3400, 0x4DBF}, // CJK Ext A
    {0x4E00, 0x9FFF}, // CJK Unified
    {0xA000, 0xA4CF}, // Yi
    {0xAC00, 0xD7A3}, // Hangul Syllables
    {0xF900, 0xFAFF}, // CJK Compatibility Ideographs
    {0xFE30, 0xFE4F}, // CJK Compatibility Forms
    {0xFF00, 0xFF60}, // Fullwidth Forms
    {0xFFE0, 0xFFE6}, // Fullwidth Signs
    {0x1F300, 0x1F64F}, // emoji blocks (pictographs, transport, modifiers)
    {0x1F900, 0x1F9FF}, // supplemental symbols
    {0x20000, 0x2FA1F}, // CJK Ext B..F
};

} // namespace

eresult<std::pair<char32_t, std::size_t>> decode_utf8(std::string_view line,
                                                      std::size_t byte) {
    if (byte >= line.size()) {
        return std::unexpected(Error{"decode: offset past end of line"});
    }
    const unsigned char b0 = static_cast<unsigned char>(line[byte]);

    // 1-byte
    if (b0 < 0x80) {
        return std::pair{static_cast<char32_t>(b0), std::size_t{1}};
    }

    auto cont = [&](std::size_t off) -> int {
        // Returns the 6 payload bits, or -1 when not a continuation byte.
        const std::size_t i = byte + off;
        if (i >= line.size()) {
            return -1;
        }
        const unsigned char b = static_cast<unsigned char>(line[i]);
        if ((b & 0xC0) != 0x80) {
            return -1;
        }
        return b & 0x3F;
    };

    char32_t cp = 0;
    std::size_t len = 0;
    char32_t min_cp = 0;

    if ((b0 & 0xE0) == 0xC0 && (b0 & 0xFE) != 0xC0) { // 110xxxxx, not 1100000x
        len = 2;
        cp = b0 & 0x1F;
        min_cp = 0x80;
    } else if ((b0 & 0xF0) == 0xE0) { // 1110xxxx
        len = 3;
        cp = b0 & 0x0F;
        min_cp = 0x800;
    } else if ((b0 & 0xF8) == 0xF0 && (b0 & 0xFC) != 0xF8) { // 11110xxx, not >10FFFF prefix
        len = 4;
        cp = b0 & 0x07;
        min_cp = 0x10000;
    } else {
        return std::unexpected(Error{"decode: invalid leading byte"});
    }

    for (std::size_t off = 1; off < len; ++off) {
        const int bits = cont(off);
        if (bits < 0) {
            return std::unexpected(Error{"decode: truncated or bad continuation"});
        }
        cp = (cp << 6) | static_cast<char32_t>(bits);
    }

    // Overlong / surrogate / out-of-range rejection.
    if (cp < min_cp || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
        return std::unexpected(Error{"decode: overlong, surrogate, or out of range"});
    }

    return std::pair{cp, len};
}

std::string encode_utf8(char32_t cp) {
    std::string out;
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}

std::uint32_t line_len_cp(std::string_view line) {
    std::uint32_t n = 0;
    std::size_t i = 0;
    while (i < line.size()) {
        const auto dec = decode_utf8(line, i);
        if (dec) {
            i += dec->second; // well-formed cp
        } else {
            ++i; // malformed byte = one pseudo-codepoint
        }
        ++n;
    }
    return n;
}

std::size_t cp_to_byte(std::string_view line, std::uint32_t col) {
    std::uint32_t n = 0;
    std::size_t i = 0;
    while (i < line.size() && n < col) {
        const auto dec = decode_utf8(line, i);
        i += dec ? dec->second : std::size_t{1};
        ++n;
    }
    return i; // clamps past-the-end naturally
}

std::uint32_t byte_to_cp(std::string_view line, std::size_t byte) {
    // The codepoint that OWNS `byte`: a byte inside a multi-byte sequence maps
    // to that sequence's codepoint index, a byte at a cp boundary to the cp
    // starting there.
    std::uint32_t n = 0;
    std::size_t i = 0;
    while (i < line.size() && i < byte) {
        const auto dec = decode_utf8(line, i);
        const std::size_t len = dec ? dec->second : std::size_t{1};
        if (i + len > byte) {
            break; // byte falls inside this cp (it owns byte)
        }
        i += len;
        ++n;
    }
    return n;
}

CharClass classify(char32_t cp) {
    if (is_whitespace_cp(cp)) {
        return CharClass::Whitespace;
    }
    if (cp < 0x80) {
        if (ascii_digit(cp) || ascii_lower(cp) || ascii_upper(cp) || cp == U'_') {
            return CharClass::Word;
        }
        return CharClass::Punct;
    }
    // Non-ASCII: letters/ideographs count as Word (v1 approximation; the
    // unicode word-boundary refinements are a P3 concern, not P1).
    return CharClass::Word;
}

bool is_blank(char32_t cp) { return classify(cp) == CharClass::Whitespace; }

bool same_word_class(char32_t a, char32_t b) {
    const CharClass ca = classify(a);
    const CharClass cb = classify(b);
    return ca != CharClass::Whitespace && ca == cb;
}

int cp_display_width(char32_t cp, int tabwidth, int disp_col) {
    if (cp == U'\t') {
        const int tw = tabwidth > 0 ? tabwidth : 8;
        const int rem = disp_col % tw;
        return tw - rem; // 1..tabwidth, never 0
    }
    if (cp >= 0x1100) {
        for (const auto& r : k_wide_ranges) {
            if (cp >= r.lo && cp <= r.hi) {
                return 2;
            }
        }
    }
    return 1;
}

} // namespace noni
