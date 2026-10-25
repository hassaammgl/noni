#pragma once
// P1.2 — buffer storage: line vector + file load/save + dirty/revision.
// Pure core: no ncurses, no app/ui headers, no exceptions (AGENTS.md).
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/pos.hpp"
#include "core/undo.hpp" // TextChange/EditKind/UndoStack (undo model, P1.4)

namespace noni {

class Buffer {
public:
    // A buffer always holds at least one line (an empty buffer is {""}).
    explicit Buffer(std::vector<std::string> lines = {});

    // Never returns an empty-buffer fallback on failure: a missing/broken file
    // is an eresult error. Uses error_code-only filesystem calls.
    [[nodiscard]] static eresult<Buffer> load(const std::filesystem::path& path);

    // Writes LF-terminated lines. Marks the buffer clean ONLY when `path`
    // equals the buffer's own path (a save-as to another name stays dirty
    // under the old identity until retargeted).
    [[nodiscard]] eresult<void> save(const std::filesystem::path& path) const;

    // --- edit primitives (P1.3). All positions are clamped; out-of-bounds
    // arguments degrade to in-bounds equivalents instead of failing. Each
    // op records a TextChange internally (undo wiring lands in P1.4).
    //
    // Insert one codepoint after `p`... i.e. AT `p`'s column.
    [[nodiscard]] Pos insert_char(Pos p, char32_t cp);
    // Insert text; '\n' inside `text` splits lines (paste path). Returns the
    // position one past the last inserted codepoint.
    [[nodiscard]] Pos insert_text(Pos p, std::string_view text);
    // Erase the codepoint AT `p` (no wrap at end of line: returns `p`).
    [[nodiscard]] Pos erase_char(Pos p);
    // Erase [range.start, range.end) (cross-line safe).
    [[nodiscard]] Pos erase_range(TextRange r);
    // Split the line at `p`; returns the new line's start (just below `p`).
    [[nodiscard]] Pos insert_newline(Pos p);
    // Join lines `p.line` and `p.line+1`. `smart` = vim-J style (collapse the
    // trailing whitespace of the first and leading whitespace of the second,
    // then one space between non-empty halves). Returns the collapsed (cursor)
    // position on the joined line; an EMPTY range (start == end with
    // line==p.line, col==0) means "no next line, nothing happened".
    [[nodiscard]] TextRange join(Pos p, bool smart);

    [[nodiscard]] const std::vector<std::string>& lines() const { return lines_; }

    // Whole-content replacement (storage-level; finer edits come in P1.3).
    void set_lines(std::vector<std::string> lines);

    [[nodiscard]] bool dirty() const { return dirty_; }
    void mark_saved() { dirty_ = false; }
    void mark_dirty() { dirty_ = true; }

    // Bumped on every content change. Undo/redo history does NOT touch the
    // revision because a group marks it via with_edit when it lands (P1.5).
    [[nodiscard]] std::uint64_t revision() const { return revision_; }

    // P1.6 — cached immutable snapshot for renderers/threads. The returned
    // shared_ptr stays stable while the buffer keeps editing (new mutations
    // rebuild the cache; holders keep the old view alive).
    [[nodiscard]] std::shared_ptr<const std::vector<std::string>> snapshot_lines();

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }
    void set_path(std::filesystem::path path) { path_ = std::move(path); }

    // Reserved for the reducer (P2+): push a caller-built change directly.
    void record_text_change(const TextChange& change);

    // --- undo/redo (P1.4). Units = one undoable edit. Cursor restore comes
    // from the unit's cur_before (undo) / cur_after (redo) hints.
    struct UndoResult {
        Pos cursor;
    };
    [[nodiscard]] eresult<UndoResult> undo();
    [[nodiscard]] eresult<UndoResult> redo();
    void begin_undo_group() { undo_.begin_group(); }
    void begin_undo_group(EditKind kind) { undo_.begin_group(kind); }
    void end_undo_group() { undo_.end_group(); }
    // P1.5 — the ONE sanctioned grouping entry point for the reducer. Nesting
    // reuses the inner group. CONTRACT: `f` must not throw (policy: no
    // exceptions anywhere); an early return inside `f` closes the group the
    // same way — the undo state stays consistent either way.
    template <class F>
    void with_edit(EditKind kind, F&& f) {
        if (undo_.group_open()) {
            f(); // nested: the inner group owns the unit
            return;
        }
        begin_undo_group(kind);
        f();
        end_undo_group();
    }
    [[nodiscard]] std::size_t undo_depth() const { return undo_.undo_depth(); }
    [[nodiscard]] std::size_t redo_depth() const { return undo_.redo_depth(); }

private:
    // Splices `change` into `lines_`, marks dirty, bumps revision, and records
    // it in the undo history (P1.4).
    void apply_change(const TextChange& change);
    // Inverse splice (undo: new->old; redo internal check uses old->new path of
    // apply_change itself). Debug-asserts content match before splicing.
    void apply_inverse(const TextChange& change);

    std::vector<std::string> lines_ = {""}; // invariant: size() >= 1
    std::filesystem::path path_;
    bool dirty_ = false;
    std::uint64_t revision_ = 0;
    UndoStack undo_;
    std::shared_ptr<const std::vector<std::string>> snapshot_cache_;
    std::uint64_t snapshot_rev_ = static_cast<std::uint64_t>(-1);
};

// Load/save helpers shared by Buffer and later modules (tests use them too).

// Splits raw file content into lines: splits on '\n', drops exactly one
// trailing empty split when the content ends with '\n', strips one '\r'
// before each '\n' (CRLF->LF), always yields >= 1 line.
[[nodiscard]] std::vector<std::string> split_file_content(std::string_view data);

// Joins lines with '\n' + trailing '\n' (round-trips split_file_content for
// files that end with a newline).
[[nodiscard]] std::string join_file_content(const std::vector<std::string>& lines);

} // namespace noni
