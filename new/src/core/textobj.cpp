// P4.4/P4.5 — see textobj.hpp. Behavior pinned against nvim: the word object
// takes trailing blanks and falls back to the preceding blanks at EOL; the
// parameter object extends into following blank lines and falls back to the
// preceding ones at EOF; quotes pair up inside the current line only; brackets
// match across lines with nesting.
#include "core/textobj.hpp"

#include <algorithm>
#include <string>

#include "core/text.hpp"

namespace noni {
namespace {

[[nodiscard]] std::uint32_t clamp_line(const std::vector<std::string>& lines,
                                       const std::uint32_t line) {
    const std::uint32_t last = static_cast<std::uint32_t>(lines.size() - 1);
    return line > last ? last : line;
}

[[nodiscard]] char32_t cp_at(const std::string& line, const std::uint32_t col) {
    const std::size_t byte = cp_to_byte(line, col);
    if (const auto d = decode_utf8(line, byte)) {
        return d->first;
    }
    return static_cast<char32_t>(
        static_cast<unsigned char>(line[std::min(byte, line.size() - 1)]));
}

[[nodiscard]] bool line_is_blank(const std::string& line) {
    return line.find_first_not_of(" \t") == std::string::npos;
}

// "a"-objects include the blanks after them (nvim: da" on `x = "hi" ;` eats
// the following space too). Nothing to do at end of line.
void extend_trailing_blanks(const std::vector<std::string>& lines,
                            TextObjectRange& r) {
    if (r.linewise || r.range.end.line >= lines.size()) {
        return;
    }
    const std::string& line = lines[r.range.end.line];
    const std::uint32_t len = line_len_cp(line);
    std::uint32_t col = r.range.end.col;
    while (col < len && is_blank(cp_at(line, col))) {
        ++col;
    }
    r.range.end.col = col;
}

// iw/aw: the word (or blank run) under the cursor.
[[nodiscard]] eresult<TextObjectRange>
word_object(const std::vector<std::string>& lines, const Pos cursor,
            const bool around) {
    const std::uint32_t l = clamp_line(lines, cursor.line);
    const std::string& line = lines[l];
    const std::uint32_t len = line_len_cp(line);
    if (len == 0) {
        return std::unexpected(Error{"text object: empty line"});
    }
    const std::uint32_t col = std::min(cursor.col, len - 1);
    const CharClass cls = classify(cp_at(line, col));

    auto same_class = [&](const std::uint32_t c) {
        if (c >= len) {
            return false;
        }
        return classify(cp_at(line, c)) == cls;
    };

    std::uint32_t start = col;
    std::uint32_t end = col + 1;
    while (start > 0 && same_class(start - 1)) {
        --start;
    }
    while (end < len && same_class(end)) {
        ++end;
    }

    if (around && cls != CharClass::Whitespace) {
        std::uint32_t after = end;
        while (after < len && is_blank(cp_at(line, after))) {
            ++after;
        }
        if (after > end) {
            end = after; // trailing blanks
        } else {
            std::uint32_t before = start;
            while (before > 0 && is_blank(cp_at(line, before - 1))) {
                --before;
            }
            start = before; // no trailing blanks: take the preceding ones
        }
    } else if (around) {
        // On whitespace vim's "aw" takes the blank run AND the following word
        // (nvim: daw on the spaces of "foo   bar baz" leaves "foo baz").
        std::uint32_t after = end;
        while (after < len && !is_blank(cp_at(line, after))) {
            ++after;
        }
        if (after == end) {
            return std::unexpected(Error{"text object: trailing blanks"});
        }
        end = after;
    }

    TextObjectRange r;
    r.range = TextRange{Pos{l, start}, Pos{l, end}};
    return r;
}

// ip/ap: maximal run of (non-)blank lines around the cursor. Around also takes
// the following blank lines, or the preceding ones when the paragraph is last.
[[nodiscard]] eresult<TextObjectRange>
para_object(const std::vector<std::string>& lines, const Pos cursor,
            const bool around) {
    const std::uint32_t l = clamp_line(lines, cursor.line);
    const bool blank = line_is_blank(lines[l]);

    std::uint32_t first = l;
    std::uint32_t last = l;
    while (first > 0 && line_is_blank(lines[first - 1]) == blank) {
        --first;
    }
    while (last + 1 < lines.size()
           && line_is_blank(lines[last + 1]) == blank) {
        ++last;
    }

    if (around) {
        std::uint32_t after = last;
        while (after + 1 < lines.size() && line_is_blank(lines[after + 1])) {
            ++after;
        }
        if (after > last) {
            last = after;
        } else {
            std::uint32_t before = first;
            while (before > 0 && line_is_blank(lines[before - 1])) {
                --before;
            }
            first = before;
        }
    }

    TextObjectRange r;
    r.linewise = true;
    r.range = TextRange{Pos{first, 0}, Pos{last + 1, 0}};
    return r;
}

// i"/a"/i'/a': quotes pair up inside the CURRENT line (vim does not cross
// lines). A cursor inside/on a pair uses it; otherwise the first pair after
// the cursor wins; no pair after the cursor = no object.
[[nodiscard]] eresult<TextObjectRange>
quote_object(const std::vector<std::string>& lines, const Pos cursor,
             const char32_t quote, const bool around) {
    const std::uint32_t l = clamp_line(lines, cursor.line);
    const std::string& line = lines[l];
    const std::uint32_t len = line_len_cp(line);
    std::vector<std::uint32_t> marks;
    for (std::uint32_t c = 0; c < len; ++c) {
        if (cp_at(line, c) == quote) {
            marks.push_back(c);
        }
    }
    if (marks.size() < 2) {
        return std::unexpected(Error{"text object: no string"});
    }
    const std::uint32_t col = len == 0 ? 0 : std::min(cursor.col, len - 1);

    std::size_t chosen = marks.size();
    for (std::size_t i = 0; i + 1 < marks.size(); i += 2) {
        if (col >= marks[i] && col <= marks[i + 1]) {
            chosen = i;
            break;
        }
    }
    if (chosen == marks.size()) {
        for (std::size_t i = 0; i + 1 < marks.size(); i += 2) {
            if (marks[i] > col) {
                chosen = i;
                break;
            }
        }
    }
    if (chosen == marks.size()) {
        return std::unexpected(Error{"text object: no string"});
    }

    const std::uint32_t open = marks[chosen];
    const std::uint32_t close = marks[chosen + 1];
    TextObjectRange r;
    r.range = around ? TextRange{Pos{l, open}, Pos{l, close + 1}}
                     : TextRange{Pos{l, open + 1}, Pos{l, close}};
    if (around) {
        extend_trailing_blanks(lines, r);
    }
    return r;
}

struct Loc {
    std::uint32_t line = 0;
    std::uint32_t col = 0;
    char32_t cp = 0;
};

[[nodiscard]] std::vector<Loc> flatten(const std::vector<std::string>& lines) {
    std::vector<Loc> out;
    for (std::uint32_t l = 0; l < lines.size(); ++l) {
        const std::string& s = lines[l];
        std::size_t byte = 0;
        std::uint32_t col = 0;
        while (byte < s.size()) {
            const auto d = decode_utf8(s, byte);
            const char32_t cp =
                d ? d->first
                  : static_cast<char32_t>(
                        static_cast<unsigned char>(s[byte]));
            const std::size_t n = d ? d->second : 1;
            out.push_back(Loc{l, col, cp});
            byte += n;
            ++col;
        }
    }
    return out;
}

// i(/a(, i{/a{, i[/a[: nesting-aware match across lines.
[[nodiscard]] eresult<TextObjectRange>
bracket_object(const std::vector<std::string>& lines, const Pos cursor,
               const char32_t open, const char32_t close, const bool around) {
    const std::vector<Loc> flat = flatten(lines);
    if (flat.empty()) {
        return std::unexpected(Error{"text object: no bracket"});
    }

    // Index of the cursor cell (clamped to the buffer).
    std::size_t cur = 0;
    const std::uint32_t l = clamp_line(lines, cursor.line);
    for (std::size_t i = 0; i < flat.size(); ++i) {
        if (flat[i].line == l && flat[i].col <= cursor.col) {
            cur = i;
        }
    }

    long open_i = -1;
    long close_i = -1;
    if (flat[cur].cp == open) {
        open_i = static_cast<long>(cur);
    } else if (flat[cur].cp == close) {
        close_i = static_cast<long>(cur);
    }

    if (open_i < 0 && close_i < 0) {
        // Nearest enclosing open: walk back over balanced pairs.
        int depth = 0;
        for (long i = static_cast<long>(cur); i >= 0; --i) {
            const char32_t cp = flat[static_cast<std::size_t>(i)].cp;
            if (cp == close) {
                ++depth;
            } else if (cp == open) {
                if (depth == 0) {
                    open_i = i;
                    break;
                }
                --depth;
            }
        }
        if (open_i < 0) {
            return std::unexpected(Error{"text object: no bracket"});
        }
    }

    if (close_i < 0) {
        int depth = 0;
        for (std::size_t i = static_cast<std::size_t>(open_i) + 1;
             i < flat.size(); ++i) {
            if (flat[i].cp == open) {
                ++depth;
            } else if (flat[i].cp == close) {
                if (depth == 0) {
                    close_i = static_cast<long>(i);
                    break;
                }
                --depth;
            }
        }
    } else {
        int depth = 0;
        for (long i = close_i - 1; i >= 0; --i) {
            const char32_t cp = flat[static_cast<std::size_t>(i)].cp;
            if (cp == close) {
                ++depth;
            } else if (cp == open) {
                if (depth == 0) {
                    open_i = i;
                    break;
                }
                --depth;
            }
        }
    }
    if (open_i < 0 || close_i < 0) {
        return std::unexpected(Error{"text object: unmatched bracket"});
    }

    const Loc& o = flat[static_cast<std::size_t>(open_i)];
    const Loc& c = flat[static_cast<std::size_t>(close_i)];
    TextObjectRange r;
    r.range = around ? TextRange{Pos{o.line, o.col}, Pos{c.line, c.col + 1}}
                     : TextRange{Pos{o.line, o.col + 1}, Pos{c.line, c.col}};
    if (around) {
        extend_trailing_blanks(lines, r);
    }
    return r;
}

} // namespace

std::optional<TextObjectId> text_object_id(const char32_t prefix,
                                           const char32_t key) {
    if (prefix != U'i' && prefix != U'a') {
        return std::nullopt;
    }
    const bool inner = prefix == U'i';
    switch (key) {
    case U'w':
        return inner ? TextObjectId::InnerWord : TextObjectId::AWord;
    case U'p':
        return inner ? TextObjectId::InnerPara : TextObjectId::APara;
    case U'"':
        return inner ? TextObjectId::InnerQuoteDouble
                     : TextObjectId::AQuoteDouble;
    case U'\'':
        return inner ? TextObjectId::InnerQuoteSingle
                     : TextObjectId::AQuoteSingle;
    case U'(':
    case U')':
        return inner ? TextObjectId::InnerParen : TextObjectId::AParen;
    case U'{':
    case U'}':
        return inner ? TextObjectId::InnerBrace : TextObjectId::ABrace;
    case U'[':
    case U']':
        return inner ? TextObjectId::InnerBracket : TextObjectId::ABracket;
    default:
        return std::nullopt;
    }
}

eresult<TextObjectRange> text_object_range(const TextObjectId id,
                                           const std::vector<std::string>& lines,
                                           const Pos cursor) {
    if (lines.empty()) {
        return std::unexpected(Error{"text object: empty buffer"});
    }
    switch (id) {
    case TextObjectId::InnerWord:
        return word_object(lines, cursor, false);
    case TextObjectId::AWord:
        return word_object(lines, cursor, true);
    case TextObjectId::InnerPara:
        return para_object(lines, cursor, false);
    case TextObjectId::APara:
        return para_object(lines, cursor, true);
    case TextObjectId::InnerQuoteDouble:
        return quote_object(lines, cursor, U'"', false);
    case TextObjectId::AQuoteDouble:
        return quote_object(lines, cursor, U'"', true);
    case TextObjectId::InnerQuoteSingle:
        return quote_object(lines, cursor, U'\'', false);
    case TextObjectId::AQuoteSingle:
        return quote_object(lines, cursor, U'\'', true);
    case TextObjectId::InnerParen:
        return bracket_object(lines, cursor, U'(', U')', false);
    case TextObjectId::AParen:
        return bracket_object(lines, cursor, U'(', U')', true);
    case TextObjectId::InnerBrace:
        return bracket_object(lines, cursor, U'{', U'}', false);
    case TextObjectId::ABrace:
        return bracket_object(lines, cursor, U'{', U'}', true);
    case TextObjectId::InnerBracket:
        return bracket_object(lines, cursor, U'[', U']', false);
    case TextObjectId::ABracket:
        return bracket_object(lines, cursor, U'[', U']', true);
    }
    return std::unexpected(Error{"text object: unknown"});
}

} // namespace noni
