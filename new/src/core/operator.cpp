#include "core/operator.hpp"

#include <algorithm>
#include <string>

#include "core/text.hpp"

namespace noni {
namespace {

[[nodiscard]] std::optional<OperatorId> operator_for_key(const Key& k) {
    if (k.kind != KeyKind::Char) {
        return std::nullopt;
    }
    switch (k.cp) {
    case U'd':
        return OperatorId::Delete;
    case U'y':
        return OperatorId::Yank;
    case U'c':
        return OperatorId::Change;
    case U'>':
        return OperatorId::ShiftRight;
    case U'<':
        return OperatorId::ShiftLeft;
    case U'=':
        return OperatorId::AutoIndent;
    default:
        return std::nullopt;
    }
}

// The key that repeats an operator: its DOUBLED form (dd/yy/cc/>>/<</==) is
// the linewise whole-line variant.
[[nodiscard]] char32_t doubled_key(const OperatorId op) {
    switch (op) {
    case OperatorId::Delete:
        return U'd';
    case OperatorId::Yank:
        return U'y';
    case OperatorId::Change:
        return U'c';
    case OperatorId::ShiftRight:
        return U'>';
    case OperatorId::ShiftLeft:
        return U'<';
    case OperatorId::AutoIndent:
        return U'=';
    default:
        return 0; // gU/gu/g~ repeat as 'g' + key; P4.7 arms them
    }
}

struct MotionSpec {
    MotionId id = MotionId::H;
    bool linewise = false;  // j k G gg: the operator works on whole lines
    bool inclusive = false; // the motion's landing char is part of the range
};

// Motions accepted right after an operator. `;`/`,` need the window's last
// find value and land with P4.2 (TODO(P4.2): route them through TextView).
[[nodiscard]] std::optional<MotionSpec> motion_for_char(const char32_t cp) {
    switch (cp) {
    case U'h':
        return MotionSpec{MotionId::H, false, false};
    case U'l':
        return MotionSpec{MotionId::L, false, false};
    case U'j':
        return MotionSpec{MotionId::J, true, false};
    case U'k':
        return MotionSpec{MotionId::K, true, false};
    case U'0':
        return MotionSpec{MotionId::Zero, false, false};
    case U'^':
        return MotionSpec{MotionId::FirstNonblank, false, false};
    case U'$':
        return MotionSpec{MotionId::Dollar, false, true};
    case U'w':
        return MotionSpec{MotionId::Word, false, false};
    case U'W':
        return MotionSpec{MotionId::WordUpper, false, false};
    case U'b':
        return MotionSpec{MotionId::WordBack, false, false};
    case U'B':
        return MotionSpec{MotionId::WordBackUpper, false, false};
    case U'e':
        return MotionSpec{MotionId::WordEnd, false, true};
    case U'E':
        return MotionSpec{MotionId::WordEndUpper, false, true};
    case U'G':
        return MotionSpec{MotionId::GotoLast, true, false};
    case U'{':
        return MotionSpec{MotionId::ParaBack, false, false};
    case U'}':
        return MotionSpec{MotionId::ParaFwd, false, false};
    case U'%':
        return MotionSpec{MotionId::MatchBracket, false, true};
    default:
        return std::nullopt;
    }
}

// f/F/t/T need one more key: the {char} to search for.
[[nodiscard]] std::optional<MotionId> find_arm_for_char(const char32_t cp) {
    switch (cp) {
    case U'f':
        return MotionId::FindFwd;
    case U'F':
        return MotionId::FindBack;
    case U't':
        return MotionId::ToFwd;
    case U'T':
        return MotionId::ToBack;
    default:
        return std::nullopt;
    }
}

// Motion flags for the find family (resolved after the target char arrives).
[[nodiscard]] MotionSpec find_spec(const MotionId id) {
    switch (id) {
    case MotionId::FindFwd:
        return MotionSpec{id, false, true};
    case MotionId::FindBack:
        return MotionSpec{id, false, false};
    case MotionId::ToFwd:
        return MotionSpec{id, false, true};
    case MotionId::ToBack:
        return MotionSpec{id, false, false};
    default:
        return MotionSpec{id, false, false};
    }
}

// One codepoint past `p` inside its line (the exclusive end of an inclusive
// motion like `d$`); an end-of-line position rolls to the next line start.
[[nodiscard]] Pos after_cp(const std::vector<std::string>& lines,
                           const Pos p) {
    const std::uint32_t len =
        static_cast<std::uint32_t>(line_len_cp(lines[p.line]));
    if (p.col + 1 <= len) {
        return Pos{p.line, p.col + 1};
    }
    return Pos{std::min<std::uint32_t>(
                   p.line + 1, static_cast<std::uint32_t>(lines.size())),
               0};
}

[[nodiscard]] Edit linewise_edit(const OperatorId op, const char32_t reg,
                                 const std::uint32_t first,
                                 const std::uint32_t end_line,
                                 const std::uint32_t count) {
    Edit e;
    e.op = op;
    e.scope = EditScope::Linewise;
    e.lines = count;
    e.range = TextRange{Pos{first, 0}, Pos{end_line, 0}};
    e.reg = reg;
    e.enter_insert = op == OperatorId::Change;
    return e;
}

// vim ':h cw': a Change on a Word motion behaves like 'ce' when the cursor is
// on a non-blank (on a blank / at EOL it stays 'dw'-like).
[[nodiscard]] MotionSpec change_word_spec(const MotionSpec spec,
                                          const TextView& tv) {
    if (spec.id != MotionId::Word && spec.id != MotionId::WordUpper) {
        return spec;
    }
    const std::string& line = (*tv.lines)[tv.cursor.line];
    const std::size_t byte = cp_to_byte(line, tv.cursor.col);
    if (byte >= line.size()) {
        return spec; // cursor on the end-of-line NUL: 'dw' semantics
    }
    const auto d = decode_utf8(line, byte);
    if (!d || is_blank(d->first)) {
        return spec;
    }
    return MotionSpec{spec.id == MotionId::Word ? MotionId::WordEnd
                                               : MotionId::WordEndUpper,
                      false, true};
}

// Builds the charwise Edit for [from, to). vim turns an EXCLUSIVE motion that
// starts AND ends in column 1 linewise ('d}' over a paragraph, 'd{' back onto
// a paragraph start): same text, but a linewise register.
[[nodiscard]] Edit motion_edit(const OperatorId op, const char32_t reg,
                               const Pos from, const Pos to,
                               const bool inclusive,
                               const std::vector<std::string>& lines) {
    if (!inclusive && from.col == 0 && to.col == 0 && from.line != to.line) {
        const std::uint32_t first = std::min(from.line, to.line);
        const std::uint32_t last = std::max(from.line, to.line);
        return linewise_edit(op, reg, first, last, last - first);
    }
    Edit e;
    e.op = op;
    e.reg = reg;
    e.enter_insert = op == OperatorId::Change;
    e.scope = EditScope::Charwise;
    if (to < from) {
        e.range = TextRange{to, from}; // backward: excludes the cursor char
    } else if (inclusive) {
        e.range = TextRange{from, after_cp(lines, to)};
    } else {
        e.range = TextRange{from, to};
    }
    return e;
}

} // namespace

std::uint32_t combined_count(const Pending& p) {
    const std::uint64_t op = p.op_count == 0 ? 1 : p.op_count;
    const std::uint64_t motion = p.motion_count == 0 ? 1 : p.motion_count;
    const std::uint64_t total = op * motion;
    // vim caps counts around 2^31; keep arithmetic far away from overflow.
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(total, 1u << 20));
}

std::optional<Edit> step(Pending& p, const Key& k, const TextView& tv) {
    if (tv.lines == nullptr || tv.lines->empty()) {
        p = Pending{};
        return std::nullopt;
    }

    // '"' register prefix: the next key is the register name.
    if (p.phase == Pending::Phase::WantReg) {
        if (k.kind == KeyKind::Char) {
            p.reg = k.cp;
        }
        p.phase = Pending::Phase::Idle;
        return std::nullopt;
    }
    if (p.phase == Pending::Phase::Idle) {
        if (k.kind == KeyKind::Char && k.cp == U'"') {
            p.phase = Pending::Phase::WantReg;
            return std::nullopt;
        }
        if (const auto op = operator_for_key(k)) {
            p.phase = Pending::Phase::Op;
            p.op = *op;
            return std::nullopt;
        }
        return std::nullopt; // not ours: nothing armed, nothing to do
    }

    // --- Phase::Op -----------------------------------------------------------
    if (k.kind == KeyKind::Esc) {
        p = Pending{}; // vim: Esc cancels the operator (no edit)
        return std::nullopt;
    }
    if (k.kind != KeyKind::Char) {
        p = Pending{}; // any non-char key aborts the sequence
        return std::nullopt;
    }

    // f/t armed: this key is the {char} target.
    if (p.want_char) {
        p.want_char = false;
        const MotionSpec spec = find_spec(p.await_char);
        const Pos from = tv.cursor;
        const auto r = Motion{spec.id, k.cp}.apply(
            MotionContext{tv.lines, tv.tabwidth}, from,
            combined_count(p), from.col);
        const char32_t reg = p.reg;
        const OperatorId op = p.op;
        p = Pending{};
        if (!r || *r == from) {
            return std::nullopt;
        }
        return motion_edit(op, reg, from, *r, spec.inclusive, *tv.lines);
    }

    // 'g' + 'g' = gg (linewise): the second key is the only completion.
    if (p.g_armed) {
        p.g_armed = false;
        if (k.cp != U'g') {
            p = Pending{};
            return std::nullopt;
        }
        // TODO(P4.2): with a count, vim's G/gg count is a LINE NUMBER, not a
        // repetition count ('2dG' deletes to line 2) — pin it against the
        // oracle when the edit is applied.
        const Pos from = tv.cursor;
        const auto r = Motion{MotionId::GotoFirst}.apply(
            MotionContext{tv.lines, tv.tabwidth}, from,
            combined_count(p), from.col);
        const char32_t reg = p.reg;
        const OperatorId op = p.op;
        p = Pending{};
        if (!r) {
            return std::nullopt;
        }
        const std::uint32_t first = std::min(from.line, r->line);
        const std::uint32_t last = std::max(from.line, r->line);
        return linewise_edit(op, reg, first, last + 1, last - first + 1);
    }

    // The operator repeated: [count] whole lines from the cursor line.
    if (const char32_t self = doubled_key(p.op); self != 0 && k.cp == self) {
        const std::uint32_t count = combined_count(p);
        const std::uint32_t first = tv.cursor.line;
        const std::uint32_t total =
            static_cast<std::uint32_t>(tv.lines->size());
        const std::uint32_t end = std::min(first + count, total);
        const char32_t reg = p.reg;
        const OperatorId op = p.op;
        p = Pending{};
        if (end <= first) {
            return std::nullopt;
        }
        Edit e = linewise_edit(op, reg, first, end, end - first);
        e.keep_indent = op == OperatorId::Change; // cc/S keep the indent
        return e;
    }

    if (k.cp == U'g') {
        p.g_armed = true;
        return std::nullopt;
    }
    if (const auto find = find_arm_for_char(k.cp)) {
        p.want_char = true;
        p.await_char = *find;
        return std::nullopt;
    }
    // Text objects (iw/aw/i(/a"...) arrive with P4.4/P4.5.
    if (k.cp == U'i' || k.cp == U'a') {
        p = Pending{};
        return std::nullopt; // TODO(P4.4): arm the text-object prefix
    }

    const auto found = motion_for_char(k.cp);
    if (!found) {
        p = Pending{}; // unknown key: the operator is dropped (beep)
        return std::nullopt;
    }
    const MotionSpec spec = p.op == OperatorId::Change
                                ? change_word_spec(*found, tv)
                                : *found;

    const Pos from = tv.cursor;
    const auto r = Motion{spec.id}.apply(MotionContext{tv.lines, tv.tabwidth},
                                         from, combined_count(p), from.col);
    const char32_t reg = p.reg;
    const OperatorId op = p.op;
    p = Pending{};
    if (!r || *r == from) {
        return std::nullopt; // failed / zero-width motion: nothing to do
    }
    const Pos to = *r;
    if (spec.linewise) {
        const std::uint32_t first = std::min(from.line, to.line);
        const std::uint32_t last = std::max(from.line, to.line);
        return linewise_edit(op, reg, first, last + 1, last - first + 1);
    }
    Edit e = motion_edit(op, reg, from, to, spec.inclusive, *tv.lines);
    if (e.range.start == e.range.end) {
        return std::nullopt;
    }
    return e;
}

} // namespace noni
