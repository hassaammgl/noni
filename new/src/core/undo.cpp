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
        commit_pending();
        return true;
    }
    return false; // inner level: outer group keeps accumulating
}

void UndoStack::commit_pending() {
    if (!pending_has_content_) {
        return;
    }
    undo_.push_back(std::move(pending_));
    pending_ = {};
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
