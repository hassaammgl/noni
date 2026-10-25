// P2.2 — see reducer.hpp.
#include "app/reducer.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>

#include "core/text.hpp"

namespace noni::app {

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
    st.mark_dirty();
}

} // namespace

Command command_for_key(const EditorState& st, const Key& k) {
    using Kind = Command::Kind;
    Command c; // kind None
    switch (st.mode) {
    case Mode::Normal:
        switch (k.kind) {
        case KeyKind::Char:
            switch (k.cp) {
            case U'x': c.kind = Kind::DeleteChar; break;
            case U'u': c.kind = Kind::Undo; break;
            case U'i':
            case U'A': c.kind = Kind::EnterInsert; break; // A at EOL: P2.7
            case U'h': c.kind = Kind::Motion; c.dx = -1; break;
            case U'l': c.kind = Kind::Motion; c.dx = 1; break;
            case U'j': c.kind = Kind::Motion; c.dy = 1; break;
            case U'k': c.kind = Kind::Motion; c.dy = -1; break;
            case U'0': c.kind = Kind::Motion; c.dx = -0x40000000; break;
            case U'$': c.kind = Kind::Motion; c.dx = 0x3FFFFFFF; break;
            default: break;
            }
            break;
        case KeyKind::CtrlChar:
            if (k.cp == U'r') {
                c.kind = Kind::Redo;
            }
            break;
        case KeyKind::ArrowUp: c.kind = Kind::Motion; c.dy = -1; break;
        case KeyKind::ArrowDown: c.kind = Kind::Motion; c.dy = 1; break;
        case KeyKind::ArrowLeft: c.kind = Kind::Motion; c.dx = -1; break;
        case KeyKind::ArrowRight: c.kind = Kind::Motion; c.dx = 1; break;
        case KeyKind::Delete: c.kind = Kind::DeleteChar; break;
        default: break;
        }
        break;
    case Mode::Insert:
        switch (k.kind) {
        case KeyKind::Char:
            c.kind = Kind::InsertChar;
            c.cp = k.cp;
            break;
        case KeyKind::Esc: c.kind = Kind::LeaveInsert; break;
        case KeyKind::Backspace: c.kind = Kind::EraseChar; break;
        case KeyKind::ArrowUp: c.kind = Kind::Motion; c.dy = -1; break;
        case KeyKind::ArrowDown: c.kind = Kind::Motion; c.dy = 1; break;
        case KeyKind::ArrowLeft: c.kind = Kind::Motion; c.dx = -1; break;
        case KeyKind::ArrowRight: c.kind = Kind::Motion; c.dx = 1; break;
        default: break;
        }
        break;
    }
    return c;
}

void reducer(EditorState& st, const Command& cmd) {
    using Kind = Command::Kind;
    switch (cmd.kind) {
    case Kind::None:
        return;

    case Kind::EnterInsert:
        st.mode = Mode::Insert;
        st.mark_dirty();
        return;

    case Kind::LeaveInsert:
        st.mode = Mode::Normal;
        // Vim rule: leaving insert backs the cursor one column (to sit ON
        // the last typed char); column 0 stays. P2.7 carries the exact edge.
        if (st.win().cursor.col > 0) {
            st.win().cursor.col -= 1;
        }
        clamp_cursor(st);
        st.mark_dirty();
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
        for (std::uint32_t i = 0; i < cmd.count; ++i) {
            const auto& lines = b->lines();
            const std::int64_t nl =
                static_cast<std::int64_t>(w.cursor.line) + cmd.dy;
            if (nl < 0 || nl >= static_cast<std::int64_t>(lines.size())) {
                break; // vertical boundary: stay (v0 clamp)
            }
            w.cursor.line = static_cast<std::uint32_t>(nl);
            const std::int64_t nc =
                static_cast<std::int64_t>(w.cursor.col) + cmd.dx;
            const std::uint32_t len = line_len_cp(lines[w.cursor.line]);
            if (nc < 0) {
                if (w.cursor.col == 0) {
                    break; // h at col 0 stays (v0 semantic, P2.7 exactness)
                }
                w.cursor.col = 0; // '0' jumps to col 0
                break;
            }
            w.cursor.col = nc > static_cast<std::int64_t>(len)
                               ? len
                               : static_cast<std::uint32_t>(nc);
        }
        clamp_cursor(st);
        st.mark_dirty();
        return;
    }

    case Kind::InsertChar:
        if (st.mode != Mode::Insert || cmd.cp == U'\n') {
            return; // Enter-in-insert is P2.6 (indent-aware)
        }
        apply_edit(st, EditKind::InsertChar, [&cmd](Buffer& b, const Pos p) {
            return b.insert_char(p, cmd.cp);
        });
        return;

    case Kind::EraseChar: {
        if (st.mode != Mode::Insert) {
            return;
        }
        const WindowState& w = st.win();
        if (w.cursor.col == 0) {
            return; // backspace across line start joins in P2.6
        }
        apply_edit(st, EditKind::EraseChar, [](Buffer& b, const Pos p) {
            // Backspace erases the cp LEFT of the cursor; erase_char at
            // {line, col-1} returns that position (= the new cursor).
            return b.erase_char(Pos{p.line, p.col - 1});
        });
        return;
    }

    case Kind::DeleteChar:
        if (st.mode != Mode::Normal) {
            return;
        }
        apply_edit(st, EditKind::EraseChar, [](Buffer& b, const Pos p) {
            // 'x': erase the cp AT the cursor (no wrap at EOL: primitive
            // returns `p` unchanged when nothing was erased).
            return b.erase_char(p);
        });
        return;
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
