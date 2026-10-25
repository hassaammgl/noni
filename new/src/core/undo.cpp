// P1.4 — see undo.hpp.
#include "core/undo.hpp"

#include <cassert>

namespace noni {

bool UndoStack::end_group() {
    assert(group_depth_ > 0 && "end_group without begin_group");
    if (group_depth_ == 0) {
        return false; // hard precondition; never unwind (no exceptions)
    }
    --group_depth_;
    if (group_depth_ == 0) {
        commit_pending(); // outer close: tag still holds the group's kind
        group_tag_ = EditKind::Bulk;
        return true;
    }
    return false; // inner level: outer group keeps accumulating
}

bool UndoStack::try_extend(UndoUnit& pending) {
    // Run-merging (P1.5): a tagged InsertChar/EraseChar group commits onto the
    // previous committed unit when they are one continued char stream.
    const TextChange* c = pending.changes.empty() ? nullptr : &pending.changes.front();
    if (c == nullptr || pending.changes.size() != 1 || !redo_.empty()
        || undo_.empty()) {
        return false;
    }
    UndoUnit& last = undo_.back();
    if (last.changes.size() != 1) {
        return false; // only single-change units extend
    }
    const TextChange& prev = last.changes.front();
    if (prev.kind != c->kind
        || (prev.kind != EditKind::InsertChar && prev.kind != EditKind::EraseChar)
        || prev.first_line != c->first_line || prev.cur_after != c->cur_before) {
        return false;
    }
    // Content continuity (shared invariant): the unit's running line content
    // must be exactly what the next change started from.
    assert(prev.new_lines == c->old_lines && "char-run merge drift");
    last.changes.front().new_lines = c->new_lines;
    last.changes.front().cur_after = c->cur_after;
    return true;
}

void UndoStack::commit_pending() {
    if (!pending_has_content_) {
        return;
    }
    pending_.tag = group_tag_;
    if (pending_.tag == EditKind::InsertChar || pending_.tag == EditKind::EraseChar) {
        // Typing/backspace stream candidate: prefer extending the last unit.
        if (try_extend(pending_)) {
            redo_.clear(); // still a fresh edit from the present
            pending_.changes.clear();
            pending_has_content_ = false;
            return;
        }
    }
    undo_.push_back(std::move(pending_));
    pending_.changes.clear();
    pending_has_content_ = false;
    while (undo_.size() > k_cap) {
        undo_.pop_front(); // evict oldest
    }
    // A fresh edit from the present always truncates the redo branch.
    redo_.clear();
}

void UndoStack::push(const TextChange& change) {
    pending_.changes.push_back(change);
    pending_has_content_ = true;
    if (!group_open()) {
        commit_pending();
    }
}

std::optional<UndoUnit> UndoStack::pop_undo() {
    if (undo_.empty()) {
        return std::nullopt;
    }
    UndoUnit u = std::move(undo_.back());
    undo_.pop_back();
    return u;
}

std::optional<UndoUnit> UndoStack::pop_redo() {
    if (redo_.empty()) {
        return std::nullopt;
    }
    UndoUnit u = std::move(redo_.back());
    redo_.pop_back();
    return u;
}

void UndoStack::push_redo(UndoUnit unit) {
    redo_.push_back(std::move(unit));
    while (redo_.size() > k_cap) {
        redo_.pop_front();
    }
}

void UndoStack::push_redo_return(UndoUnit unit) {
    undo_.push_back(std::move(unit));
}

} // namespace noni
