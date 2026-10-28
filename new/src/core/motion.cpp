// P2.7 + P3 — see motion.hpp.
#include "core/motion.hpp"

#include <algorithm>
#include <expected>

namespace noni {

namespace {

// One codepoint at cp-col `col`, or U'\0' past the end (sentinel: callers
// only read it while col < len).
[[nodiscard]] char32_t cp_at_col(const std::string& line,
                                 const std::uint32_t col) {
    const std::size_t byte = cp_to_byte(line, col);
    if (byte >= line.size()) {
        return U'\0';
    }
    const auto dec = decode_utf8(line, byte);
    return dec ? dec->first : static_cast<char32_t>(line[byte]);
}

// One vim `w`/`W` step from `from` (big = WORD mode: blank-vs-nonblank only).
// Ground truth: tests/diff/motion_p3.fxt. Summary of the decode:
//   Phase A: cursor on a non-blank -> consume the cursor's same-class run
//            (line-local; runs never cross a line). If that ends on a
//            non-blank of a DIFFERENT class, land there ("foo.bar baz":
//            mid-word -> '.'; '.' -> 'b').
//   Phase B: blanks (incl. the whole rest of the line) are skipped across
//            lines; an EMPTY line (len 0) is a word and STOPS the move at
//            its col 0; a blank-only line (len>0) is skipped through unless
//            the travel would leave the buffer.
//   EOF fail: landing is the LAST CHAR of the current (last) line, cursor
//            unchanged when it is already the last char or the line is empty.
[[nodiscard]] Pos word_forward_step(const std::vector<std::string>& lines,
                                    const Pos from, const bool big) {
    const auto len_of = [&](const std::uint32_t l) {
        return static_cast<std::uint32_t>(line_len_cp(lines[l]));
    };
    const auto is_blank_cp = [&](const char32_t cp) {
        return big ? (cp == U' ' || cp == U'\t') : is_blank(cp);
    };
    std::uint32_t l = from.line;
    std::uint32_t col = from.col;

    // Phase A: consume the cursor's class run (only when it starts ON one).
    std::uint32_t len = len_of(l);
    if (col < len) {
        const char32_t first = cp_at_col(lines[l], col);
        if (!is_blank_cp(first)) {
            const CharClass cl = classify(first); // big: run = non-blanks
            while (col < len) {
                const char32_t cp = cp_at_col(lines[l], col);
                if (big ? is_blank_cp(cp) : (classify(cp) != cl)) {
                    break;
                }
                ++col;
            }
            // Ran into a different-class char inside the line: land there,
            // UNLESS it is a blank (then Phase B must skip it first).
            if (col < len && !is_blank_cp(cp_at_col(lines[l], col))) {
                return Pos{l, col};
            }
        }
    }

    // Phase B: skip blanks (across line boundaries) to the next word start.
    for (;;) {
        len = len_of(l);
        while (col < len && is_blank_cp(cp_at_col(lines[l], col))) {
            ++col;
        }
        if (col < len) {
            return Pos{l, col}; // first non-blank (often after a line cross)
        }
        if (l + 1 >= lines.size()) {
            // EOF fail: last char of this line; on an empty line: stay col 0.
            return len == 0 ? Pos{l, 0} : Pos{l, len - 1};
        }
        if (len_of(l + 1) == 0) {
            return Pos{l + 1, 0}; // empty line is a word stop
        }
        ++l;
        col = 0;
    }
}

// One vim `b`/`B` (big) step from `from` (P3.2). Fixture: motion_p3b.fxt.
// Summary of the decode:
//   Phase A: cursor on NON-BLANK, walk back within the line to the START of
//            its class run ("foo.bar baz" mid-"bar" -> 'b'; word START ->
//            no step here, Phase B takes over).
//   Phase B: scan backward: blanks skipped (a blank-only line is run
//            through), an EMPTY line (len 0) is a word and STOPS at col 0;
//            on the first non-blank, walk to the start of ITS class run.
//   BOF fail: cursor stays exactly where it was.
[[nodiscard]] Pos word_back_step(const std::vector<std::string>& lines,
                                 const Pos from, const bool big) {
    const auto len_of = [&](const std::uint32_t l) {
        return static_cast<std::uint32_t>(line_len_cp(lines[l]));
    };
    const auto is_blank_cp = [&](const char32_t cp) {
        return big ? (cp == U' ' || cp == U'\t') : is_blank(cp);
    };
    std::uint32_t l = from.line;
    std::uint32_t col = from.col;
    const std::uint32_t len = len_of(l);

    // Phase A: cursor inside a run, but not at its start -> run start.
    if (col < len && !is_blank_cp(cp_at_col(lines[l], col))) {
        const CharClass cl = classify(cp_at_col(lines[l], col));
        while (col > 0) {
            const char32_t prev = cp_at_col(lines[l], col - 1);
            if (big ? is_blank_cp(prev) : (classify(prev) != cl)) {
                break;
            }
            --col;
        }
        if (col != from.col) {
            return Pos{l, col};
        }
    }

    // Phase B: travel backward to the previous word's start.
    std::uint32_t j = l;
    std::int64_t c = static_cast<std::int64_t>(col) - 1;
    for (;;) {
        if (c < 0) {
            if (j == 0) {
                return from; // no previous word: BOF fail, cursor stays
            }
            --j;
            const std::uint32_t plen = len_of(j);
            if (plen == 0) {
                return Pos{j, 0}; // empty line is a word stop
            }
            c = static_cast<std::int64_t>(plen) - 1;
            continue;
        }
        const char32_t cp = cp_at_col(lines[j], static_cast<std::uint32_t>(c));
        if (is_blank_cp(cp)) {
            --c;
            continue;
        }
        // First non-blank: walk to the start of its class run.
        const CharClass cl = classify(cp);
        while (c > 0) {
            const char32_t prev =
                cp_at_col(lines[j], static_cast<std::uint32_t>(c - 1));
            if (big ? is_blank_cp(prev) : (classify(prev) != cl)) {
                break;
            }
            --c;
        }
        return Pos{j, static_cast<std::uint32_t>(c)};
    }
}

// One vim `e`/`E` (big) step from `from` (P3.2). Fixture: motion_p3b.fxt.
// Summary of the decode:
//   Phase A: cursor on NON-BLANK, walk forward within the line to the END of
//            its class run ("ab" mid-word -> 'b'; at the run END already ->
//            Phase B).
//   Phase B: scan FORWARD: blanks skipped (blank-only and EMPTY lines are
//            run through — e does NOT stop on empty lines), on the first
//            non-blank walk to the END of ITS class run.
//   EOF fail: last char of the last line, or col 0 when that line is empty
//            (probe e_to_empty_last_line lands ON the empty last line).
[[nodiscard]] Pos word_end_step(const std::vector<std::string>& lines,
                                const Pos from, const bool big) {
    const auto len_of = [&](const std::uint32_t l) {
        return static_cast<std::uint32_t>(line_len_cp(lines[l]));
    };
    const auto is_blank_cp = [&](const char32_t cp) {
        return big ? (cp == U' ' || cp == U'\t') : is_blank(cp);
    };
    std::uint32_t l = from.line;
    std::uint32_t col = from.col;
    const std::uint32_t len = len_of(l);

    // Phase A: cursor inside a run, but not at its end -> run end.
    if (col < len && !is_blank_cp(cp_at_col(lines[l], col))) {
        const CharClass cl = classify(cp_at_col(lines[l], col));
        while (col + 1 < len) {
            const char32_t next = cp_at_col(lines[l], col + 1);
            if (big ? is_blank_cp(next) : (classify(next) != cl)) {
                break;
            }
            ++col;
        }
        if (col != from.col) {
            return Pos{l, col};
        }
    }

    // Phase B: travel forward to the next word's end.
    std::uint32_t j = l;
    std::int64_t c = static_cast<std::int64_t>(col) + 1;
    for (;;) {
        if (c >= static_cast<std::int64_t>(len_of(j))) {
            if (j + 1 >= lines.size()) {
                const std::uint32_t elen = len_of(j);
                return elen == 0 ? Pos{j, 0} : Pos{j, elen - 1};
            }
            ++j;
            c = 0;
            continue;
        }
        const char32_t cp = cp_at_col(lines[j], static_cast<std::uint32_t>(c));
        if (is_blank_cp(cp)) {
            ++c;
            continue;
        }
        // First non-blank: walk to the end of its class run.
        const CharClass cl = classify(cp);
        while (c + 1 < static_cast<std::int64_t>(len_of(j))) {
            const char32_t next =
                cp_at_col(lines[j], static_cast<std::uint32_t>(c + 1));
            if (big ? is_blank_cp(next) : (classify(next) != cl)) {
                break;
            }
            ++c;
        }
        return Pos{j, static_cast<std::uint32_t>(c)};
    }
}

// P3.5 — one `{`/`}` paragraph-boundary step. Fixture: motion_p3e.fxt.
// Decode summary:
//   The ONLY separators are EMPTY lines (len 0) — a blank-only ('   ') line
//   is NOT a boundary (the oracle steps keep walking through it); the step
//   scans the lines below/above the cursor until the first empty one and
//   lands on its char 0 (a separator is its own paragraph start).
//   All-boundary-less travel fails at the buffer end exactly like the word
//   motions: EOF -> last char of the last line (col 0 on an empty last
//   line); BOF -> (line 0, col 0). Empty single-line buffer: no-op.
[[nodiscard]] Pos para_step(const std::vector<std::string>& lines,
                            const Pos from, const bool forward) {
    const std::int64_t n = static_cast<std::int64_t>(lines.size());
    const std::int64_t dir = forward ? 1 : -1;
    for (std::int64_t l = static_cast<std::int64_t>(from.line) + dir;
         l >= 0 && l < n; l += dir) {
        if (line_len_cp(lines[static_cast<std::size_t>(l)]) == 0) {
            return Pos{static_cast<std::uint32_t>(l), 0};
        }
    }
    if (forward) {
        const std::size_t ll = static_cast<std::size_t>(n - 1);
        const std::uint32_t len =
            static_cast<std::uint32_t>(line_len_cp(lines[ll]));
        return Pos{static_cast<std::uint32_t>(ll), len == 0 ? 0 : len - 1};
    }
    return Pos{0, 0};
}

// P3.3 — one find attempt (f/F/t/T). Current line only; the search NEVER// includes the cursor's own col; with count N the N-th occurrence must
// exist or the whole motion FAILS (no partial move, oracle-pinned), so the
// count is consumed in one scan instead of a step loop.
//   f/F: land ON the occurrence; t/T: one char before it (t, right side) or
//   after it (T, left side). t with an adjacent target can be a no-move
//   success (oracle: "ax" + tx stays on col 0).
[[nodiscard]] eresult<Pos> find_step(const std::vector<std::string>& lines,
                                     const Pos from, const MotionId id,
                                     const char32_t cp,
                                     const std::uint32_t count) {
    const auto len_of = [&](const std::uint32_t l) {
        return static_cast<std::uint32_t>(line_len_cp(lines[l]));
    };
    const std::uint32_t len = len_of(from.line);
    const bool forward = id == MotionId::FindFwd || id == MotionId::ToFwd;
    const bool to = id == MotionId::ToFwd || id == MotionId::ToBack;
    std::uint32_t seen = 0;

    if (forward) {
        for (std::uint32_t col = from.col + 1; col < len; ++col) {
            if (cp_at_col(lines[from.line], col) == cp) {
                if (++seen == count) {
                    return to ? Pos{from.line, col - 1}
                              : Pos{from.line, col};
                }
            }
        }
    } else {
        // col signed: the scan may reach 0.
        for (std::int64_t col = static_cast<std::int64_t>(from.col) - 1;
             col >= 0; --col) {
            if (cp_at_col(lines[from.line], static_cast<std::uint32_t>(col))
                == cp) {
                if (++seen == count) {
                    return to ? Pos{from.line,
                                    static_cast<std::uint32_t>(col) + 1}
                              : Pos{from.line, static_cast<std::uint32_t>(col)};
                }
            }
        }
    }
    return std::unexpected(Error{"find target not found"});
}

// P3.7 — the six default vim 'matchpairs' characters (no '<>' and no config).
[[nodiscard]] bool is_bracket(const char32_t cp) {
    return cp == U'(' || cp == U')' || cp == U'[' || cp == U']'
        || cp == U'{' || cp == U'}';
}

[[nodiscard]] bool is_opener(const char32_t cp) {
    return cp == U'(' || cp == U'[' || cp == U'{';
}

// The counterpart of an opening/closing bracket (0 for a non-bracket).
[[nodiscard]] char32_t bracket_partner(const char32_t cp) {
    switch (cp) {
    case U'(':
        return U')';
    case U')':
        return U'(';
    case U'[':
        return U']';
    case U']':
        return U'[';
    case U'{':
        return U'}';
    case U'}':
        return U'{';
    default:
        return 0;
    }
}

// '%' — one bracket match step. Decode (oracle: motion_p3g.fxt; the algorithm
// mirrors nvim's findmatchlimit for the default 'matchpairs'):
//   1. A bracket UNDER the cursor is matched.
//   2. Otherwise the FIRST bracket to its right ON THE SAME LINE is matched
//      (the scan never crosses the line end — "abc" / "(x)" does not move).
//   3. No bracket at all -> error (no move).
//   4. Matching counts ONLY the same bracket type, forward for an opener and
//      backward for a closer; every other bracket is neutral, so "([)]"
//      matches '(' with ')'. Running off the buffer -> error (no move).
// Deliberate limitations (ROADMAP P3.7): comments/strings are NOT skipped and
// there is no '#if'/'#endif' matching.
[[nodiscard]] eresult<Pos> match_bracket_step(
    const std::vector<std::string>& lines, const Pos from) {
    const auto len_of = [&](const std::uint32_t l) {
        return static_cast<std::uint32_t>(line_len_cp(lines[l]));
    };
    const auto cp_at = [&](const std::uint32_t l, const std::uint32_t c) {
        return cp_at_col(lines[l], c);
    };

    std::uint32_t col = from.col;
    char32_t bracket = col < len_of(from.line) ? cp_at(from.line, col) : 0;
    if (!is_bracket(bracket)) {
        const std::uint32_t len = len_of(from.line);
        for (std::uint32_t c = col + 1; c < len; ++c) {
            if (is_bracket(cp_at(from.line, c))) {
                bracket = cp_at(from.line, c);
                col = c;
                break;
            }
        }
    }
    if (!is_bracket(bracket)) {
        return std::unexpected(Error{"no bracket in this line"});
    }

    const char32_t partner = bracket_partner(bracket);
    std::uint32_t depth = 1;
    if (is_opener(bracket)) {
        std::uint32_t l = from.line;
        std::uint32_t c = col;
        for (;;) {
            if (c + 1 < len_of(l)) {
                ++c;
            } else if (l + 1 < lines.size()) {
                ++l;
                c = 0;
            } else {
                break; // end of buffer
            }
            const char32_t cur = cp_at(l, c);
            if (cur == bracket) {
                ++depth;
            } else if (cur == partner && --depth == 0) {
                return Pos{l, c};
            }
        }
    } else {
        std::uint32_t l = from.line;
        std::uint32_t c = col;
        for (;;) {
            if (c > 0) {
                --c;
            } else if (l > 0) {
                --l;
                const std::uint32_t plen = len_of(l);
                if (plen == 0) {
                    continue; // empty line: keep walking backwards
                }
                c = plen - 1;
            } else {
                break; // start of buffer
            }
            const char32_t cur = cp_at(l, c);
            if (cur == bracket) {
                ++depth;
            } else if (cur == partner && --depth == 0) {
                return Pos{l, c};
            }
        }
    }
    return std::unexpected(Error{"no matching bracket"});
}

} // namespace

eresult<Pos> Motion::apply(const MotionContext& ctx, const Pos from,
                           const std::uint32_t count,
                           const std::uint32_t desired) const {
    if (ctx.lines == nullptr || ctx.lines->empty()) {
        return from;
    }
    const auto& lines = *ctx.lines;
    // Clamps `to` onto [0, last-char] for its LINE (empty line -> col 0).
    const auto clamp_norm = [&](const Pos p) {
        const std::uint32_t len =
            static_cast<std::uint32_t>(line_len_cp((*ctx.lines)[p.line]));
        if (len == 0) {
            return Pos{p.line, 0};
        }
        return Pos{p.line, std::min(p.col, len - 1)};
    };

    switch (id) {
    case MotionId::H: {
        const std::uint32_t back = std::min(from.col, count);
        return clamp_norm(Pos{from.line, from.col - back});
    }
    case MotionId::L: {
        const std::uint32_t len =
            static_cast<std::uint32_t>(line_len_cp(lines[from.line]));
        const std::uint32_t last = len == 0 ? 0 : len - 1;
        return Pos{from.line,
                   static_cast<std::uint32_t>(std::min<std::uint64_t>(
                       static_cast<std::uint64_t>(from.col) + count,
                       static_cast<std::uint64_t>(last)))};
    }
    case MotionId::J:
    case MotionId::K: {
        // Oracle (j_col_clamp_persists_via_k_back): the col clamp does NOT
        // erase the aim — `desired` is remembered across shorter lines and
        // the NEXT vertical move recovers it (clamped again).
        const bool down = id == MotionId::J;
        const std::int64_t target = down
            ? static_cast<std::int64_t>(from.line) + count
            : static_cast<std::int64_t>(from.line) - count;
        const std::uint32_t nl = static_cast<std::uint32_t>(std::clamp(
            target, static_cast<std::int64_t>(0),
            static_cast<std::int64_t>(lines.size() - 1)));
        return clamp_norm(Pos{nl, desired});
    }
    case MotionId::Zero:
        return Pos{from.line, 0};
    case MotionId::Dollar: {
        // vim N$: end of line (from.line + count - 1), clamped to the last
        // line. Oracle quirk (tests/diff probes n2..n8): when the START is
        // already the last line (no downward move is possible at all), the
        // whole command FAILS — cursor stays exactly where it was (even a
        // mid-line col stays).
        const std::size_t n = lines.size();
        if (count <= 1) {
            const std::uint32_t len = static_cast<std::uint32_t>(
                line_len_cp(lines[from.line]));
            return Pos{from.line, len == 0 ? 0 : len - 1};
        }
        std::uint64_t target = static_cast<std::uint64_t>(from.line)
                             + static_cast<std::uint64_t>(count) - 1;
        if (target > n - 1) {
            target = n - 1;
        }
        if (target == from.line) {
            return from; // oracle: overflow with no downward move rejects
        }
        const std::uint32_t len = static_cast<std::uint32_t>(
            line_len_cp(lines[static_cast<std::size_t>(target)]));
        return Pos{static_cast<std::uint32_t>(target),
                   len == 0 ? 0 : len - 1};
    }
    case MotionId::Word:
    case MotionId::WordUpper: {
        // Repeat the single step; a step that cannot advance (EOB) also
        // ends every later step at the same place -> break early (5w on a
        // 2-word line ends on the last word, oracle-probed).
        const bool big = id == MotionId::WordUpper;
        Pos p = from;
        for (std::uint32_t i = 0; i < count; ++i) {
            const Pos next = word_forward_step(lines, p, big);
            if (next == p) {
                break;
            }
            p = next;
        }
        return p;
    }
    case MotionId::WordBack:
    case MotionId::WordBackUpper: {
        // `b`edging toward BOF: a no-op step ends the loop (cursor stays).
        const bool big = id == MotionId::WordBackUpper;
        Pos p = from;
        for (std::uint32_t i = 0; i < count; ++i) {
            const Pos next = word_back_step(lines, p, big);
            if (next == p) {
                break;
            }
            p = next;
        }
        return p;
    }
    case MotionId::WordEnd:
    case MotionId::WordEndUpper: {
        // `e` at the file end repeats the fail-landing (stays); loop breaks.
        const bool big = id == MotionId::WordEndUpper;
        Pos p = from;
        for (std::uint32_t i = 0; i < count; ++i) {
            const Pos next = word_end_step(lines, p, big);
            if (next == p) {
                break;
            }
            p = next;
        }
        return p;
    }
    case MotionId::FindFwd:
    case MotionId::FindBack:
    case MotionId::ToFwd:
    case MotionId::ToBack: {
        return find_step(lines, from, id, arg, count);
    }
    case MotionId::FirstNonblank: {
        // '^' — first non-blank of the current line. Oracle decode: count
        // is IGNORED (caret_count_*: 2^ stays on this line); on a blank-only
        // line the cursor rests on the LAST char; empty line -> col 0.
        const std::uint32_t len =
            static_cast<std::uint32_t>(line_len_cp(lines[from.line]));
        for (std::uint32_t col = 0; col < len; ++col) {
            if (!is_blank(cp_at_col(lines[from.line], col))) {
                return Pos{from.line, col};
            }
        }
        return Pos{from.line, len == 0 ? 0 : len - 1};
    }
    case MotionId::GotoFirst:
    case MotionId::GotoLast: {
        // gg / G — oracle decode (motion_p3d): the col is CARRIED like a
        // vertical move (desired: the Gk probe lands "bottom" col 2 and k
        // recovers it on "  longer") — NOT the first non-blank. gg: line
        // [count] (default 1); G: line [count], default the LAST line.
        // Both clamp to the file (never a partial-fail like Dollar).
        const std::uint64_t last_line = lines.size() - 1;
        std::uint64_t target = count <= 1 ? 0 : count - 1;
        if (id == MotionId::GotoLast && count <= 1) {
            target = last_line;
        }
        if (target > last_line) {
            target = last_line;
        }
        return clamp_norm(
            Pos{static_cast<std::uint32_t>(target), desired});
    }
    case MotionId::ParaFwd:
    case MotionId::ParaBack: {
        // '}'/'{' — one paragraph-boundary step; repeat per count (2{ ==
        // two single steps, oracle-pinned).
        const bool forward = id == MotionId::ParaFwd;
        Pos p = from;
        for (std::uint32_t i = 0; i < count; ++i) {
            const Pos next = para_step(lines, p, forward);
            if (next == p) {
                break;
            }
            p = next;
        }
        return p;
    }
    case MotionId::MatchBracket:
        // '%' without a count: bracket match (the count form is PercentLine,
        // chosen by the reducer while the count prefix is still pending).
        return match_bracket_step(lines, from);
    case MotionId::PercentLine: {
        // [count]'%' — nvim: lnum = (line_count * count + 99) / 100 (rounded
        // up so 100% always lands on the last line), count > 100 is rejected
        // (beep: no move). A jump to the SAME line leaves the cursor alone;
        // otherwise the column is kept and clamped into the target line.
        if (count > 100) {
            return std::unexpected(Error{"percent out of range"});
        }
        const std::uint64_t n = lines.size();
        std::uint64_t lnum = (n * count + 99) / 100;
        lnum = std::clamp<std::uint64_t>(lnum, 1, n);
        const auto target = static_cast<std::uint32_t>(lnum - 1);
        if (target == from.line) {
            return from;
        }
        const std::uint32_t len =
            static_cast<std::uint32_t>(line_len_cp(lines[target]));
        return Pos{target, len == 0 ? 0 : std::min(from.col, len - 1)};
    }
    }
    return from;
}

} // namespace noni
