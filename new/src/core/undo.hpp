#pragma once
// P1.4 — undo history model + stack. Pure data; the Buffer owns application
// (see buffer.cpp). No exceptions; no bare C handles.
//
// Model: the history is a stack of Units. One Unit = one undoable edit (a
// typing run, one command like `dw`, one ex substitution batch). Each change
// inside a unit is a line-span replacement.
//
// Undo applies change inverses in REVERSE order (each change was computed
// against the state left by its predecessors), redo re-applies them forward.
#include <cstdint>
#include <deque>
#include <optional>
#include <utility>
#include <vector>

#include "core/pos.hpp"

namespace noni {

// Coarse edit kinds for undo grouping. Typing runs merge (see P1.4/1.5);
// everything else says Bulk.
enum class EditKind : std::uint8_t {
    InsertChar, // single-codepoint insertion (typing runs)
    EraseChar,  // single-codepoint erase (backspace/delete runs)
    Bulk,       // everything else (ranges, multi-line, paste, replace)
};

// One applied edit expressed at LINE granularity: lines
// [first_line, first_line+old_lines.size()) had `old_lines` before and
// `new_lines` after the change. `cur_before`/`cur_after` are optional cursor
// hints (used by undo to restore cursor position).
struct TextChange {
    EditKind kind = EditKind::Bulk;
    std::uint32_t first_line = 0;
    std::vector<std::string> old_lines; // content BEFORE the change
    std::vector<std::string> new_lines; // content AFTER the change
    Pos cur_before;
    Pos cur_after;
};

// One undoable edit: changes in FORWARD application order. `tag` mirrors the
// with_edit group label (innermost wins when nested; informational).
struct UndoUnit {
    std::vector<TextChange> changes;
    EditKind tag = EditKind::Bulk;
};

class UndoStack {
public:
    static constexpr std::size_t k_cap = 200;

    // --- grouping -----------------------------------------------------------
    // Nested begin/end reuse the innermost logical group (P1.5 contract).
    void begin_group() { ++group_depth_; }
    void begin_group(EditKind kind) { ++group_depth_; group_tag_ = kind; }
    // Returns true when the OUTER level closed and pending changes were
    // committed as one unit.
    bool end_group();
    [[nodiscard]] bool group_open() const { return group_depth_ > 0; }

    // Record a change. With an open group it is appended to the pending unit;
    // otherwise it commits as its own unit — OR extends the previous committed
    // unit when it continues a char-level run (typing/backspace streams,
    // P1.5): same kind, same line, cursor hint continuity, no redo branch.
    void push(const TextChange& change);

    // --- history queries/plumbing -------------------------------------------
    [[nodiscard]] std::optional<UndoUnit> pop_undo(); // unit to invert
    [[nodiscard]] std::optional<UndoUnit> pop_redo(); // unit to reapply
    void push_redo(UndoUnit unit);                    // park after undo
    void push_redo_return(UndoUnit unit);             // re-commit after redo
    void clear_redo() { redo_.clear(); }

    [[nodiscard]] std::size_t undo_depth() const { return undo_.size(); }
    [[nodiscard]] std::size_t redo_depth() const { return redo_.size(); }

private:
    void commit_pending();
    bool try_extend(UndoUnit& pending);

    std::deque<UndoUnit> undo_; // back = most recent
    std::deque<UndoUnit> redo_; // back = next redo
    UndoUnit pending_;
    std::size_t group_depth_ = 0;
    bool pending_has_content_ = false;
    EditKind group_tag_ = EditKind::Bulk;
};

} // namespace noni
