// P1.4 — see undo.hpp.
#include "core/undo.hpp"

#include <algorithm>
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
        || prev.cur_after != c->cur_before) {
        return false;
    }
    // P2.6 (nvim oracle `ihello<CR>world<Esc>u`): a typing run crosses the
    // Enter split -- the whole insert stream is ONE undo. InsertChar changes
    // may target a different line than the run's head; char-level deletes
    // (EraseChar) stay line-local.
    if (prev.kind != EditKind::InsertChar && prev.first_line != c->first_line) {
        return false;
    }
    // Region splice: `c`'s old region must sit exactly inside `prev`'s new
    // content (rel = line offset within the replaced span). This doubles as
    // the content-continuity check (the old same-line drift assert).
    if (c->first_line < prev.first_line) {
        return false;
    }
    const std::size_t rel =
        static_cast<std::size_t>(c->first_line)
        - static_cast<std::size_t>(prev.first_line);
    if (rel + c->old_lines.size() > prev.new_lines.size()
        || !std::equal(c->old_lines.begin(), c->old_lines.end(),
                       prev.new_lines.begin()
                           + static_cast<std::ptrdiff_t>(rel))) {
        return false; // non-contiguous run: commit separately (safe fallback)
    }
    auto& m = last.changes.front();
    const auto at = m.new_lines.begin() + static_cast<std::ptrdiff_t>(rel);
    m.new_lines.erase(at, at + static_cast<std::ptrdiff_t>(c->old_lines.size()));
    m.new_lines.insert(at, c->new_lines.begin(), c->new_lines.end());
    m.cur_after = c->cur_after;
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
