#pragma once
// P4.4/P4.5 — text objects: pure range resolution for the word/paragraph
// family (P4.4) and the quote/bracket family (P4.5). Ranges are charwise
// [start, end) in codepoint positions; a linewise object reports linewise so
// the operator keeps whole lines (register linewise, `nap` counts lines).
#include <cstdint>
#include <optional>
#include <vector>

#include "core/pos.hpp"

namespace noni {

enum class TextObjectId : std::uint8_t {
    InnerWord,        // iw
    AWord,            // aw
    InnerPara,        // ip
    APara,            // ap
    InnerQuoteDouble, // i"
    AQuoteDouble,     // a"
    InnerQuoteSingle, // i'
    AQuoteSingle,     // a'
    InnerParen,       // i( i)
    AParen,           // a( a)
    InnerBrace,       // i{ i}
    ABrace,           // a{ a}
    InnerBracket,     // i[ i]
    ABracket,         // a[ a]
};

struct TextObjectRange {
    TextRange range{};
    bool linewise = false;
};

// `prefix` is 'i' (inner) or 'a' (around); `key` is the object keystroke.
// nullopt means the pair is not a text object (the operator cancels).
[[nodiscard]] std::optional<TextObjectId>
text_object_id(char32_t prefix, char32_t key);

// Resolve `id` at `cursor`. Error = no object here (unmatched delimiter,
// no string on the line): the caller cancels without editing.
[[nodiscard]] eresult<TextObjectRange>
text_object_range(TextObjectId id, const std::vector<std::string>& lines,
                  Pos cursor);

} // namespace noni
