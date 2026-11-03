#include "core/operator.hpp"

#include <algorithm>
#include <string>

#include <cwctype>

#include "core/text.hpp"
#include "core/textobj.hpp"

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
    case OperatorId::Upper:
        return U'U'; // P4.7: gUU / gUgU
    case OperatorId::Lower:
        return U'u'; // P4.7: guu
    case OperatorId::ToggleCase:
        return U'~'; // P4.7: g~~
    default:
        return 0;
    }
}

struct MotionSpec {
    MotionId id = MotionId::H;
    bool linewise = false;  // j k G gg: the operator works on whole lines
    bool inclusive = false; // the motion's landing char is part of the range
};

[[nodiscard]] bool is_case_op(const OperatorId op) {
    return op == OperatorId::Upper || op == OperatorId::Lower
        || op == OperatorId::ToggleCase;
}

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

    // 'g' + 'g' = gg (linewise); for the case operators 'g' + their own doubled
    // key is the whole-line form (gUgU / gugu / g~g~).
    if (p.g_armed) {
        p.g_armed = false;
        if (k.cp == doubled_key(p.op) && is_case_op(p.op)) {
            const std::uint32_t count = combined_count(p);
            const std::uint32_t first = tv.cursor.line;
            const std::uint32_t total =
                static_cast<std::uint32_t>(tv.lines->size());
            const std::uint32_t last = std::min(first + count, total) - 1;
            const char32_t reg = p.reg;
            const OperatorId op = p.op;
            p = Pending{};
            return linewise_edit(op, reg, first, last + 1, last - first + 1);
        }
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
    // Text objects (iw/aw/ip/ap, quotes, brackets): the prefix arms, the next
    // key names the object.
    if (p.want_object) {
        p.want_object = false;
        const char32_t prefix = p.object_prefix;
        const std::optional<TextObjectId> id = text_object_id(prefix, k.cp);
        const char32_t reg = p.reg;
        const OperatorId op = p.op;
        p = Pending{};
        if (!id) {
            return std::nullopt;
        }
        const auto r = text_object_range(*id, *tv.lines, tv.cursor);
        if (!r || r->range.start == r->range.end) {
            return std::nullopt; // unmatched: no edit, count already consumed
        }
        Edit e;
        e.op = op;
        e.reg = reg;
        e.enter_insert = op == OperatorId::Change;
        if (r->linewise) {
            e.scope = EditScope::Linewise;
            e.lines = r->range.end.line - r->range.start.line;
            e.range = TextRange{Pos{r->range.start.line, 0},
                                Pos{r->range.end.line, 0}};
            e.keep_indent = op == OperatorId::Change;
        } else {
            e.scope = EditScope::Charwise;
            e.range = r->range;
        }
        return e;
    }
    if (k.cp == U'i' || k.cp == U'a') {
        p.want_object = true;
        p.object_prefix = k.cp;
        return std::nullopt;
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
    if (!r) {
        return std::nullopt; // failed motion: nothing to do
    }
    const Pos to = *r;
    if (to == from) {
        // An INCLUSIVE motion that does not move still owns the char under
        // the cursor (`d$`/`cw` on the last char of a line delete it). An
        // empty line has no such char.
        if (!spec.inclusive) {
            return std::nullopt;
        }
        if (from.col >= line_len_cp((*tv.lines)[from.line])) {
            return std::nullopt;
        }
    }
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

namespace {

// Leading blank prefix of `line` (bytes) and its length in codepoints.
[[nodiscard]] std::string indent_prefix(const std::string& line) {
    std::size_t byte = 0;
    while (byte < line.size()) {
        const auto d = decode_utf8(line, byte);
        if (!d || (d->first != U' ' && d->first != U'\t')) {
            break;
        }
        byte += d->second;
    }
    return line.substr(0, byte);
}

[[nodiscard]] std::uint32_t first_nonblank_col(const std::string& line) {
    return byte_to_cp(line, indent_prefix(line).size());
}

// P11 owns 'expandtab'/'shiftwidth'; until then follow the buffer's own style.
[[nodiscard]] std::string indent_unit(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        if (line.empty()) {
            continue;
        }
        if (line.front() == '\t') {
            return "\t";
        }
        if (line.front() == ' ') {
            return std::string(static_cast<std::size_t>(k_default_tabwidth), ' ');
        }
    }
    return std::string(static_cast<std::size_t>(k_default_tabwidth), ' ');
}

// Lines a Charwise/Linewise edit actually touches [first, last] (inclusive).
[[nodiscard]] std::pair<std::uint32_t, std::uint32_t>
affected_lines(const std::vector<std::string>& lines, const Edit& e,
               const TextRange& r) {
    const std::uint32_t total = static_cast<std::uint32_t>(lines.size());
    const std::uint32_t first = std::min(r.start.line, total - 1);
    std::uint32_t last = first;
    if (e.scope == EditScope::Linewise) {
        const std::uint32_t n = e.lines == 0 ? 1 : e.lines;
        last = std::min(first + n - 1, total - 1);
    } else {
        last = r.end.line >= total ? total - 1 : r.end.line;
        if (r.end.col == 0 && r.end.line > r.start.line && last > first) {
            --last; // the end sits on the next line's first char
        }
    }
    return {first, last};
}

[[nodiscard]] Register extract_register(const std::vector<std::string>& lines,
                                        const Edit& e, const TextRange& r) {
    Register reg;
    const std::uint32_t total = static_cast<std::uint32_t>(lines.size());
    if (e.scope == EditScope::Linewise) {
        const auto [first, last] = affected_lines(lines, e, r);
        reg.lines.assign(lines.begin() + first, lines.begin() + last + 1);
        reg.linewise = true;
        return reg;
    }
    if (r.start.line == r.end.line) {
        const std::string& line = lines[r.start.line];
        const std::size_t b0 = cp_to_byte(line, r.start.col);
        const std::size_t b1 = cp_to_byte(line, r.end.col);
        reg.lines = {line.substr(b0, b1 > b0 ? b1 - b0 : 0)};
        return reg;
    }
    const std::string& first_line = lines[r.start.line];
    reg.lines.push_back(first_line.substr(cp_to_byte(first_line, r.start.col)));
    for (std::uint32_t l = r.start.line + 1; l < r.end.line && l < total; ++l) {
        reg.lines.push_back(lines[l]);
    }
    if (r.end.line < total) {
        const std::string& last_line = lines[r.end.line];
        reg.lines.push_back(last_line.substr(0, cp_to_byte(last_line, r.end.col)));
    }
    return reg;
}

// The single line-span change that ERASES the edit's range ('d' and 'c').
[[nodiscard]] TextChange erase_change(const std::vector<std::string>& lines,
                                      const Edit& e, const TextRange& r) {
    TextChange ch;
    ch.kind = EditKind::Bulk; // operator ranges are one undo unit each
    ch.cur_before = r.start;
    const std::uint32_t total = static_cast<std::uint32_t>(lines.size());

    if (e.scope == EditScope::Linewise) {
        const auto [first, last] = affected_lines(lines, e, r);
        const std::uint32_t n = last - first + 1;
        ch.first_line = first;
        ch.old_lines.assign(lines.begin() + first, lines.begin() + last + 1);
        if (e.keep_indent) {
            ch.new_lines = {indent_prefix(ch.old_lines.front())};
            ch.cur_after = Pos{first, line_len_cp(ch.new_lines.front())};
        } else if (n == total) {
            ch.new_lines = {""}; // a buffer always keeps one line
            ch.cur_after = Pos{0, 0};
        } else if (last + 1 < total) {
            ch.cur_after = Pos{first, first_nonblank_col(lines[first + n])};
        } else {
            ch.cur_after = Pos{first - 1, first_nonblank_col(lines[first - 1])};
        }
        return ch;
    }

    ch.first_line = r.start.line;
    if (r.start.line == r.end.line) {
        const std::string& line = lines[r.start.line];
        const std::size_t b0 = cp_to_byte(line, r.start.col);
        const std::size_t b1 = cp_to_byte(line, r.end.col);
        ch.old_lines = {line};
        ch.new_lines = {line.substr(0, b0) + (b1 >= b0 ? line.substr(b1) : "")};
    } else {
        const std::uint32_t end_line = std::min(r.end.line, total);
        const std::string& first_line = lines[r.start.line];
        ch.old_lines.assign(lines.begin() + r.start.line,
                            end_line < total ? lines.begin() + end_line + 1
                                             : lines.end());
        std::string merged =
            first_line.substr(0, cp_to_byte(first_line, r.start.col));
        if (end_line < total) {
            const std::string& last_line = lines[end_line];
            merged += last_line.substr(cp_to_byte(last_line, r.end.col));
        }
        ch.new_lines = {std::move(merged)};
    }
    ch.cur_after = r.start;
    return ch;
}

[[nodiscard]] std::string shift_left_one(const std::string& line,
                                         const std::size_t unit_bytes) {
    if (!line.empty() && line.front() == '\t') {
        return line.substr(1);
    }
    std::size_t spaces = 0;
    while (spaces < line.size() && spaces < unit_bytes && line[spaces] == ' ') {
        ++spaces;
    }
    return line.substr(spaces);
}

[[nodiscard]] TextChange shift_change(const std::vector<std::string>& lines,
                                      const Edit& e, const TextRange& r,
                                      const bool left) {
    const auto [first, last] = affected_lines(lines, e, r);
    const std::string unit = indent_unit(lines);
    TextChange ch;
    ch.kind = EditKind::Bulk;
    ch.first_line = first;
    ch.old_lines.assign(lines.begin() + first, lines.begin() + last + 1);
    ch.cur_before = r.start;
    ch.new_lines.reserve(ch.old_lines.size());
    for (const std::string& line : ch.old_lines) {
        ch.new_lines.push_back(left ? shift_left_one(line, unit.size())
                                    : unit + line);
    }
    ch.cur_after = Pos{first, first_nonblank_col(ch.new_lines.front())};
    return ch;
}

// Net nesting delta of one line (crude c-family heuristic; P4.8 promises a
// heuristic, not nvim's indentexpr).
[[nodiscard]] int net_depth(const std::string& line) {
    int depth = 0;
    const std::size_t body = indent_prefix(line).size();
    for (std::size_t i = body; i < line.size(); ++i) {
        switch (line[i]) {
        case '{':
        case '(':
        case '[':
            ++depth;
            break;
        case '}':
        case ')':
        case ']':
            --depth;
            break;
        default:
            break;
        }
    }
    return depth;
}

[[nodiscard]] TextChange indent_change(const std::vector<std::string>& lines,
                                       const Edit& e, const TextRange& r) {
    const auto [first, last] = affected_lines(lines, e, r);
    const std::string unit = indent_unit(lines);
    int depth = 0;
    for (std::uint32_t i = 0; i < first; ++i) {
        depth += net_depth(lines[i]);
    }
    if (depth < 0) {
        depth = 0;
    }

    TextChange ch;
    ch.kind = EditKind::Bulk;
    ch.first_line = first;
    ch.old_lines.assign(lines.begin() + first, lines.begin() + last + 1);
    ch.cur_before = r.start;
    ch.new_lines.reserve(ch.old_lines.size());
    for (const std::string& line : ch.old_lines) {
        const std::string body = line.substr(indent_prefix(line).size());
        int line_depth = depth;
        if (!body.empty() && (body.front() == '}'
                              || body.front() == ')' || body.front() == ']')) {
            line_depth = depth > 0 ? depth - 1 : 0;
        }
        std::string out;
        for (int k = 0; k < line_depth; ++k) {
            out += unit;
        }
        out += body;
        ch.new_lines.push_back(std::move(out));
        depth += net_depth(body);
        if (depth < 0) {
            depth = 0;
        }
    }
    ch.cur_after = Pos{first, first_nonblank_col(ch.new_lines.front())};
    return ch;
}

[[nodiscard]] std::string transform_case(const std::string& line,
                                         std::size_t b0, std::size_t b1,
                                         const OperatorId op) {
    b1 = std::min(b1, line.size());
    b0 = std::min(b0, b1);
    std::string out;
    out.reserve(line.size());
    out += line.substr(0, b0);
    std::size_t byte = b0;
    while (byte < b1) {
        const auto d = decode_utf8(line, byte);
        if (!d) {
            out += line[byte];
            ++byte;
            continue;
        }
        char32_t cp = d->first;
        const std::wint_t wide = static_cast<std::wint_t>(cp);
        switch (op) {
        case OperatorId::Upper:
            cp = static_cast<char32_t>(std::towupper(wide));
            break;
        case OperatorId::Lower:
            cp = static_cast<char32_t>(std::towlower(wide));
            break;
        default: {
            // g~: swap the case of every letter.
            const std::wint_t up = std::towupper(wide);
            const std::wint_t low = std::towlower(wide);
            if (wide == up && wide != low) {
                cp = static_cast<char32_t>(low);
            } else if (wide == low && wide != up) {
                cp = static_cast<char32_t>(up);
            }
            break;
        }
        }
        out += encode_utf8(cp);
        byte += d->second;
    }
    out += line.substr(b1);
    return out;
}

[[nodiscard]] TextChange case_change(const std::vector<std::string>& lines,
                                     const Edit& e, const TextRange& r) {
    const auto [first, last] = affected_lines(lines, e, r);
    TextChange ch;
    ch.kind = EditKind::Bulk;
    ch.first_line = first;
    ch.old_lines.assign(lines.begin() + first, lines.begin() + last + 1);
    ch.cur_before = r.start;
    ch.new_lines.reserve(ch.old_lines.size());
    for (std::uint32_t i = first; i <= last; ++i) {
        const std::string& line = lines[i];
        std::size_t b0 = 0;
        std::size_t b1 = line.size();
        if (e.scope == EditScope::Charwise) {
            if (i == r.start.line) {
                b0 = cp_to_byte(line, r.start.col);
            }
            if (i == r.end.line) {
                b1 = cp_to_byte(line, r.end.col);
            }
        }
        ch.new_lines.push_back(transform_case(line, b0, b1, e.op));
    }
    ch.cur_after = r.start;
    return ch;
}

} // namespace

eresult<EditPlan> plan_edit(const std::vector<std::string>& lines,
                            const Edit& e, const Pos cursor) {
    if (lines.empty()) {
        return std::unexpected(Error{"operator: empty buffer"});
    }
    EditPlan out;
    TextRange r = e.range;
    if (r.end < r.start) {
        std::swap(r.start, r.end);
    }
    const std::uint32_t total = static_cast<std::uint32_t>(lines.size());
    if (r.start.line >= total) {
        return std::unexpected(Error{"operator: range past EOF"});
    }
    if (r.end.line > total) {
        r.end.line = total;
    }

    switch (e.op) {
    case OperatorId::Yank:
        out.reg = extract_register(lines, e, r);
        // vim: a linewise yank does not move the cursor at all; a charwise one
        // lands on the start of the yanked text.
        out.cursor_after = e.scope == EditScope::Linewise
                               ? Pos{std::min(r.start.line, total - 1), cursor.col}
                               : Pos{std::min(r.start.line, total - 1), r.start.col};
        return out;
    case OperatorId::Delete:
    case OperatorId::Change:
        out.reg = extract_register(lines, e, r);
        out.change = erase_change(lines, e, r);
        out.cursor_after = out.change.cur_after;
        out.changes_text = true;
        return out;
    case OperatorId::ShiftRight:
        out.change = shift_change(lines, e, r, false);
        out.cursor_after = out.change.cur_after;
        out.changes_text = true;
        return out;
    case OperatorId::ShiftLeft:
        out.change = shift_change(lines, e, r, true);
        out.cursor_after = out.change.cur_after;
        out.changes_text = true;
        return out;
    case OperatorId::AutoIndent:
        out.change = indent_change(lines, e, r);
        out.cursor_after = out.change.cur_after;
        out.changes_text = true;
        return out;
    case OperatorId::Upper:
    case OperatorId::Lower:
    case OperatorId::ToggleCase:
        out.change = case_change(lines, e, r);
        out.cursor_after = out.change.cur_after;
        out.changes_text = true;
        return out;
    }
    return std::unexpected(Error{"operator: unknown"});
}

} // namespace noni
