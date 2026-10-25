// P2.2 — see reducer.hpp.
#include "app/reducer.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>

#include "core/text.hpp"

namespace noni::app {

namespace {

// FindKind -> the motion that "executes" it; `,` swaps the direction
// (f<->F, t<->T). P3.3.
[[nodiscard]] MotionId find_motion_id(const FindKind k, const bool reverse) {
    switch (k) {
    case FindKind::Fwd:
        return reverse ? MotionId::FindBack : MotionId::FindFwd;
    case FindKind::Back:
        return reverse ? MotionId::FindFwd : MotionId::FindBack;
    case FindKind::ToFwd:
        return reverse ? MotionId::ToBack : MotionId::ToFwd;
    case FindKind::ToBack:
        return reverse ? MotionId::ToFwd : MotionId::ToBack;
    case FindKind::None:
        break;
    }
    return MotionId::H; // unreachable; FindKind::None never repeats
}

} // namespace

// Clamps the active window cursor onto the buffer's grid (exported in the
// header: App and the resize path reuse it).
void clamp_cursor(EditorState& st) {
    WindowState& w = st.win();
    Buffer* b = st.buffer(w.buffer);
    if (b == nullptr) {
        return;
    }
    const auto& lines = b->lines();
    w.cursor.line = std::min(w.cursor.line,
                             static_cast<std::uint32_t>(lines.size() - 1));
    const std::uint32_t len = line_len_cp(lines[w.cursor.line]);
    w.cursor.col = std::min(w.cursor.col, len);
}

namespace {

// vim curswant: every cursor-setting edit re-anchors the aim column; only
// vertical motions (J/K) consume it without resetting it (see core/motion).
void sync_desired(EditorState& st) {
    st.win().desired_col = st.win().cursor.col;
    st.mark_dirty();
}

// Runs one grouped edit (the ONLY grouping entry: Buffer::with_edit) and
// moves the cursor to the position the primitive returned.
void apply_edit(EditorState& st, EditKind kind,
                const std::function<Pos(Buffer&, Pos)>& edit) {
    WindowState& w = st.win();
    Buffer* b = st.buffer(w.buffer);
    if (b == nullptr) {
        return;
    }
    Pos after = w.cursor;
    b->with_edit(kind, [&] { after = edit(*b, w.cursor); });
    w.cursor = after;
    sync_desired(st);
}

// Applies one caller-built line-span change (the P0-reserved
// record_text_change hook) under the SAME regex-tagged grouping entry as the
// primitives, and moves the cursor. Reducer-built composite edits (Enter
// with indent copy, cross-line backspace join, paste splits) arrive here as
// ONE TextChange so they merge vim-style.
void record_change(EditorState& st, EditKind tag, const TextChange& ch,
                   const Pos after) {
    Buffer* b = st.buffer(st.win().buffer);
    if (b == nullptr) {
        return;
    }
    b->with_edit(tag, [&] { b->record_text_change(ch); });
    st.win().cursor = after;
    sync_desired(st);
}

// Erases the single cp LEFT of (l, col) as one TextChange. Hints follow the
// CURSOR trail (col_before -> col-1), not the delete position — the chain is
// what keeps backspace runs mergeable across the join.
TextChange erase_left_change(const Buffer& b, const Pos cur) {
    const std::string line = b.lines()[cur.line];
    const std::size_t b_at = cp_to_byte(line, cur.col - 1);
    std::size_t cp_bytes = 1;
    if (const auto d = decode_utf8(line, b_at)) {
        cp_bytes = d->second;
    }
    TextChange ch;
    ch.kind = EditKind::EraseChar;
    ch.first_line = cur.line;
    ch.old_lines = {line};
    ch.new_lines = {line.substr(0, b_at)
                    + line.substr(b_at + cp_bytes)};
    ch.cur_before = cur;
    ch.cur_after = Pos{cur.line, cur.col - 1};
    return ch;
}

// Leading whitespace of `line` as (codepoint count, utf8 prefix).
std::pair<std::uint32_t, std::string> line_indent(const std::string& line) {
    std::size_t byte = 0;
    std::uint32_t cps = 0;
    while (byte < line.size()) {
        const auto d = decode_utf8(line, byte);
        if (!d) {
            break; // malformed byte: not indent we can reason about
        }
        const char32_t c = d->first;
        if (c != U' ' && c != U'\t') {
            break;
        }
        byte += d->second;
        ++cps;
    }
    return {cps, line.substr(0, byte)};
}

} // namespace

Command command_for_key(const EditorState& st, const Key& k) {
    using Kind = Command::Kind;
    Command c; // kind None
    switch (st.mode) {
    case Mode::Normal:
        // P3.4: while gg is armed, only the SECOND 'g' completes it.
        if (st.win().gg_pending) {
            if (k.kind == KeyKind::Char && k.cp == U'g') {
                c.kind = Kind::Motion;
                c.motion = MotionId::GotoFirst;
            } else {
                c.kind = Kind::GgCancel;
            }
            break;
        }
        // P3.3: while a find is pending, the NEXT printable key is the
        // {char} target (digits included — they are not a count here).
        if (st.win().find_pending != FindKind::None) {
            if (k.kind == KeyKind::Char) {
                c.kind = Kind::FindTarget;
                c.cp = k.cp;
            } else {
                // Esc / any non-char aborts the pending find (vim discards
                // the armed count with it).
                c.kind = Kind::FindCancel;
            }
            break;
        }
        switch (k.kind) {
        case KeyKind::Char:
            if (k.cp >= U'1' && k.cp <= U'9') {
                // Nonzero digits always extend the count prefix; a pending
                // count makes even '0' a digit (oracle: '20' stays pending).
                c.kind = Kind::CountDigit;
                c.cp = k.cp;
                break;
            }
            if (k.cp == U'0') {
                if (st.count_prefix != 0) {
                    c.kind = Kind::CountDigit;
                    c.cp = U'0';
                    break;
                }
                c.kind = Kind::Motion; // leading '0' = the col-0 motion
                c.motion = MotionId::Zero;
                break;
            }
            switch (k.cp) {
            case U'x': c.kind = Kind::DeleteChar; break;
            case U'u': c.kind = Kind::Undo; break;
            case U'i': c.kind = Kind::EnterInsert; break;
            case U'A': c.kind = Kind::EnterInsert; c.at_eol = true; break;
            case U'h': c.kind = Kind::Motion; c.motion = MotionId::H; break;
            case U'l': c.kind = Kind::Motion; c.motion = MotionId::L; break;
            case U'j': c.kind = Kind::Motion; c.motion = MotionId::J; break;
            case U'k': c.kind = Kind::Motion; c.motion = MotionId::K; break;
            case U'$': c.kind = Kind::Motion; c.motion = MotionId::Dollar; break;
            case U'w': c.kind = Kind::Motion; c.motion = MotionId::Word; break;
            case U'W': c.kind = Kind::Motion; c.motion = MotionId::WordUpper; break;
            case U'b': c.kind = Kind::Motion; c.motion = MotionId::WordBack; break;
            case U'B': c.kind = Kind::Motion; c.motion = MotionId::WordBackUpper; break;
            case U'e': c.kind = Kind::Motion; c.motion = MotionId::WordEnd; break;
            case U'E': c.kind = Kind::Motion; c.motion = MotionId::WordEndUpper; break;
            case U'f': c.kind = Kind::FindArm; c.find_kind = FindKind::Fwd; break;
            case U'F': c.kind = Kind::FindArm; c.find_kind = FindKind::Back; break;
            case U't': c.kind = Kind::FindArm; c.find_kind = FindKind::ToFwd; break;
            case U'T': c.kind = Kind::FindArm; c.find_kind = FindKind::ToBack; break;
            case U'g': c.kind = Kind::GgArm; break;
            case U'G': c.kind = Kind::Motion; c.motion = MotionId::GotoLast; break;
            case U'^': c.kind = Kind::Motion; c.motion = MotionId::FirstNonblank; break;
            case U';': case U',': {
                // Repeat (or reverse-repeat) the remembered find. Empty
                // last_find: no command at all (oracle: stays put).
                const FindState& lf = st.win().last_find;
                if (lf.kind == FindKind::None) {
                    break;
                }
                c.kind = Kind::Motion;
                c.motion = find_motion_id(lf.kind, k.cp == U',');
                c.cp = static_cast<char32_t>(lf.cp);
                break;
            }
            default: break;
            }
            break;
        case KeyKind::CtrlChar:
            if (k.cp == U'r') {
                c.kind = Kind::Redo;
            }
            break;
        case KeyKind::ArrowUp: c.kind = Kind::Motion; c.motion = MotionId::K; break;
        case KeyKind::ArrowDown: c.kind = Kind::Motion; c.motion = MotionId::J; break;
        case KeyKind::ArrowLeft: c.kind = Kind::Motion; c.motion = MotionId::H; break;
        case KeyKind::ArrowRight: c.kind = Kind::Motion; c.motion = MotionId::L; break;
        case KeyKind::Delete: c.kind = Kind::DeleteChar; break;
        default: break;
        }
        break;
    case Mode::Insert:
        switch (k.kind) {
        case KeyKind::Char:
            if (k.cp == U'\n') {
                // Paste data: plain split, no indent tricks (P2.6); Enter
                // itself arrives as KeyKind::Enter.
                c.kind = Kind::SplitLine;
            } else {
                c.kind = Kind::InsertChar;
                c.cp = k.cp; // includes '\t' as a literal tab
            }
            break;
        case KeyKind::Enter:
            // Paste data '\r'/'\n' bytes decode to Enter — inside a paste
            // session they are plain splits, not indent-aware newline.
            c.kind = st.pasting ? Kind::SplitLine : Kind::InsertNewline;
            break;
        case KeyKind::Tab: c.kind = Kind::InsertChar; c.cp = U'\t'; break;
        case KeyKind::PasteBegin: c.kind = Kind::PasteBegin; break;
        case KeyKind::PasteEnd: c.kind = Kind::PasteEnd; break;
        case KeyKind::Esc: c.kind = Kind::LeaveInsert; break;
        case KeyKind::Backspace: c.kind = Kind::EraseChar; break;
        case KeyKind::ArrowUp: c.kind = Kind::Motion; c.motion = MotionId::K; break;
        case KeyKind::ArrowDown: c.kind = Kind::Motion; c.motion = MotionId::J; break;
        case KeyKind::ArrowLeft: c.kind = Kind::Motion; c.motion = MotionId::H; break;
        case KeyKind::ArrowRight: c.kind = Kind::Motion; c.motion = MotionId::L; break;
        default: break;
        }
        break;
    }
    return c;
}

void reducer(EditorState& st, const Command& cmd) {
    using Kind = Command::Kind;
    // Count prefix lifecycle: digits extend it; any other command consumes
    // it (None also aborts it, vim aborts on unmapped keys). FindArm/find
    // cancel stages must NOT consume it — the count belongs to the COMPLETE
    // f{char} (oracle: 2fa moves to the 2nd occurrence).
    std::uint32_t count = 1;
    if (cmd.kind == Kind::CountDigit) {
        st.count_prefix = st.count_prefix * 10
                        + static_cast<std::uint32_t>(cmd.cp - U'0');
        return;
    }
    if (cmd.kind != Kind::None && cmd.kind != Kind::FindArm
        && cmd.kind != Kind::FindCancel && cmd.kind != Kind::GgArm
        && cmd.kind != Kind::GgCancel) {
        count = st.count_prefix == 0 ? 1 : st.count_prefix;
        st.count_prefix = 0;
    }
    switch (cmd.kind) {
    case Kind::None:
        return;

    case Kind::GgArm:
        st.win().gg_pending = true;
        return;

    case Kind::GgCancel:
        st.win().gg_pending = false;
        st.count_prefix = 0;
        return;

    case Kind::FindArm:
        st.win().find_pending = cmd.find_kind;
        return;

    case Kind::FindCancel:
        st.win().find_pending = FindKind::None;
        st.count_prefix = 0;
        return;

    case Kind::FindTarget: {
        // The count arrived with the arm key and was consumed above,
        // WindowState::last_find is REMEMBERED even when the scan fails
        // (oracle: "failed_find_remembered": ; repeats the failed target).
        WindowState& w = st.win();
        const FindKind pending = w.find_pending;
        w.find_pending = FindKind::None;
        if (pending == FindKind::None) {
            return;
        }
        w.last_find.cp = static_cast<std::uint32_t>(cmd.cp);
        w.last_find.kind = pending;
        Buffer* b = st.buffer(w.buffer);
        if (b == nullptr) {
            return;
        }
        const Motion motion{find_motion_id(pending, false), cmd.cp};
        const MotionContext ctx{&b->lines(), k_default_tabwidth};
        const auto r = motion.apply(ctx, w.cursor, count, w.desired_col);
        if (r) {
            w.cursor = *r;
            w.desired_col = w.cursor.col;
        } else {
            st.msg = r.error().msg;
        }
        st.mark_dirty();
        return;
    }

    case Kind::EnterInsert:
        st.mode = Mode::Insert;
        if (cmd.at_eol) {
            // 'A': append position = one past the last char (len; empty
            // line -> 0). Oracle: A on "ab" then typing appends "abX".
            WindowState& w = st.win();
            if (Buffer* b = st.buffer(w.buffer); b != nullptr) {
                w.cursor.col = static_cast<std::uint32_t>(
                    line_len_cp(b->lines()
                                    [std::min(w.cursor.line,
                                              static_cast<std::uint32_t>(
                                                  b->lines().size() - 1))]));
            }
        }
        sync_desired(st);
        return;

    case Kind::LeaveInsert:
        st.mode = Mode::Normal;
        // Vim rule: leaving insert backs the cursor one column (to sit ON
        // the last typed char); column 0 stays. P2.7 carries the exact edge.
        if (st.win().cursor.col > 0) {
            st.win().cursor.col -= 1;
        }
        clamp_cursor(st);
        sync_desired(st);
        return;

    case Kind::Undo:
    case Kind::Redo: {
        if (st.mode != Mode::Normal) {
            return;
        }
        Buffer* b = st.buffer(st.win().buffer);
        if (b == nullptr) {
            return;
        }
        const auto r = cmd.kind == Kind::Undo ? b->undo() : b->redo();
        if (r) {
            st.win().cursor = r->cursor;
            st.msg.clear();
        } else {
            st.msg = r.error().msg;
        }
        st.mark_dirty();
        return;
    }

    case Kind::Motion: {
        WindowState& w = st.win();
        Buffer* b = st.buffer(w.buffer);
        if (b == nullptr
            || (st.mode != Mode::Normal && st.mode != Mode::Insert)) {
            return;
        }
        // P2.7: the command carries a MotionId; core/motion applies the
        // vim-exact clamps against the oracle fixture. Find motions can
        // FAIL (no occurrence): error -> message, cursor untouched (P3.3).
        const Motion motion{cmd.motion, cmd.cp};
        const MotionContext ctx{&b->lines(), k_default_tabwidth};
        const auto r = motion.apply(ctx, w.cursor, count, w.desired_col);
        if (!r) {
            st.msg = r.error().msg;
        } else {
            w.cursor = *r;
            if (cmd.motion != MotionId::J && cmd.motion != MotionId::K
                && cmd.motion != MotionId::GotoFirst
                && cmd.motion != MotionId::GotoLast) {
                // Vertical moves keep the aim; everything else re-anchors.
                w.desired_col = w.cursor.col;
            }
        }
        w.gg_pending = false; // any completed motion closes the armed 'g'
        st.mark_dirty();
        return;
    }

    case Kind::DeleteChar: {
        if (st.mode != Mode::Normal) {
            return;
        }
        WindowState& w = st.win();
        Buffer* b = st.buffer(w.buffer);
        if (b == nullptr) {
            return;
        }
        const std::uint32_t l = std::min(
            w.cursor.line, static_cast<std::uint32_t>(b->lines().size() - 1));
        const std::string line = b->lines()[l];
        const std::uint32_t len =
            static_cast<std::uint32_t>(line_len_cp(line));
        if (len == 0) {
            return; // nothing to delete on an empty line
        }
        // Oracle: x at the line end deletes the LAST char — clamp col into
        // [0, len-1] first, then span-erase up to `count` chars as ONE
        // change (one undo unit). The cursor normalizes INTO the shrunken
        // line (x at EOL: col collapses to the new last char, vim-exact).
        const std::uint32_t col = std::min(w.cursor.col, len - 1);
        const std::uint32_t span = std::min(count, len - col);
        if (span == 0) {
            return;
        }
        const std::string out = line.substr(0, cp_to_byte(line, col))
                              + line.substr(cp_to_byte(line, col + span));
        const std::uint32_t new_len =
            static_cast<std::uint32_t>(line_len_cp(out));
        TextChange ch;
        ch.kind = EditKind::EraseChar;
        ch.first_line = l;
        ch.old_lines = {line};
        ch.new_lines = {out};
        ch.cur_before = Pos{l, col};
        ch.cur_after = Pos{l, new_len == 0 ? 0 : std::min(col, new_len - 1)};
        record_change(st, EditKind::EraseChar, ch, ch.cur_after);
        return;
    }

    case Kind::InsertChar:
        if (st.mode != Mode::Insert || cmd.cp == U'\n') {
            return; // '\n' as data arrives via SplitLine; Enter via EnterKey
        }
        apply_edit(st, EditKind::InsertChar, [&cmd](Buffer& b, const Pos p) {
            return b.insert_char(p, cmd.cp);
        });
        return;

    case Kind::InsertNewline: {
        // P2.6 (nvim oracle): Enter continues the typing run — one TextChange
        // built here (P0-reserved record hook), one line-span swap.
        if (st.mode != Mode::Insert) {
            return;
        }
        const WindowState& w = st.win();
        Buffer* b = st.buffer(w.buffer);
        if (b == nullptr) {
            return;
        }
        const std::uint32_t l = w.cursor.line;
        const std::string line = b->lines()[l];
        const std::size_t head_b = cp_to_byte(line, w.cursor.col);
        const auto [indent_cps, indent_str] = line_indent(line);
        TextChange ch;
        ch.kind = EditKind::InsertChar; // keeps the typing run mergeable
        ch.first_line = l;
        ch.old_lines = {line};
        ch.new_lines = {line.substr(0, head_b), indent_str + line.substr(head_b)};
        ch.cur_before = w.cursor;
        ch.cur_after = Pos{l + 1, indent_cps};
        record_change(st, EditKind::InsertChar, ch, ch.cur_after);
        return;
    }

    case Kind::SplitLine: {
        // '\n' inside a bracketed paste: exact split, no indent copy.
        if (st.mode != Mode::Insert) {
            return;
        }
        const WindowState& w = st.win();
        Buffer* b = st.buffer(w.buffer);
        if (b == nullptr) {
            return;
        }
        const std::uint32_t l = w.cursor.line;
        const std::string line = b->lines()[l];
        const std::size_t head_b = cp_to_byte(line, w.cursor.col);
        TextChange ch;
        ch.kind = EditKind::InsertChar; // paste stream stays part of the run
        ch.first_line = l;
        ch.old_lines = {line};
        ch.new_lines = {line.substr(0, head_b), line.substr(head_b)};
        ch.cur_before = w.cursor;
        ch.cur_after = Pos{l + 1, 0};
        record_change(st, EditKind::InsertChar, ch, ch.cur_after);
        return;
    }

    case Kind::PasteBegin:
        st.pasting = true;
        st.mark_dirty();
        return;

    case Kind::PasteEnd:
        st.pasting = false;
        st.mark_dirty();
        return;

    case Kind::EraseChar: {
        if (st.mode != Mode::Insert) {
            return;
        }
        const WindowState& w = st.win();
        if (w.cursor.col == 0) {
            // P2.6: backspace at line start JOINS with the previous line
            // (one cross-line TextChange; no smart-indent deletion until P6).
            Buffer* b = st.buffer(w.buffer);
            if (b == nullptr || w.cursor.line == 0) {
                return;
            }
            const std::uint32_t l = w.cursor.line;
            const std::string prev = b->lines()[l - 1];
            const std::string cur = b->lines()[l];
            TextChange ch;
            ch.kind = EditKind::EraseChar; // backspace run keeps merging
            ch.first_line = l - 1;
            ch.old_lines = {prev, cur};
            ch.new_lines = {prev + cur};
            ch.cur_before = w.cursor;
            ch.cur_after = Pos{l - 1,
                               static_cast<std::uint32_t>(
                                   line_len_cp(prev))};
            record_change(st, EditKind::EraseChar, ch, ch.cur_after);
            return;
        }
        const WindowState& wc = st.win();
        Buffer* bc = st.buffer(wc.buffer);
        if (bc == nullptr) {
            return;
        }
        const TextChange ch = erase_left_change(*bc, wc.cursor);
        record_change(st, EditKind::EraseChar, ch, ch.cur_after);
        return;
    }

    case Kind::CountDigit:
        return; // handled before the switch (unreachable case for -Wswitch)
    }
}

void follow_cursor(EditorState& st, int view_rows) {
    if (view_rows <= 0) {
        return;
    }
    WindowState& w = st.win();
    // Minimal follow (P3.6 = full scroll set: scrolloff/zz/keep-hscroll).
    // v0 scrolloff is 0 (vim default).
    const std::uint32_t top_before = w.top_line;
    if (w.cursor.line < w.top_line) {
        w.top_line = w.cursor.line;
    }
    if (w.cursor.line >= w.top_line + static_cast<std::uint32_t>(view_rows)) {
        w.top_line = w.cursor.line - (static_cast<std::uint32_t>(view_rows)
                                          - 1);
    }
    if (w.top_line != top_before) {
        st.mark_dirty();
    }
}

void resize_clamp(EditorState& st, int view_rows) {
    clamp_cursor(st);
    follow_cursor(st, view_rows);
}

} // namespace noni::app
