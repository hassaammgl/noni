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

namespace noni {

// Coarse edit kinds for undo grouping. Typing runs merge (see P1.4/1.5);
// says Bulk otherwise.
enum class EditKind : std::uint8_t {
    InsertChar, // single-codepoint insertion (typing runs)
    EraseChar,  // single-codepoint erase (backspace/delete runs)
    Bulk,       // everything else (ranges, multi-line, paste, replace)
};

// One applied edit expressed at LINE granularity: lines
// [first_line, first_line+old_lines.size()) had `old_lines` before and
// `new_lines` after the change. `cur_before`/`cur_after` are optional cursor
// hints (used by undo to restore cursor position, P1.4).
struct TextChange {
    EditKind kind = EditKind::Bulk;
    std::uint32_t first_line = 0;
    std::vector<std::string> old_lines; // content BEFORE the change
    std::vector<std::string> new_lines; // content AFTER the change
    Pos cur_before;
    Pos cur_after;
};

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

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }
    void set_path(std::filesystem::path path) { path_ = std::move(path); }

    // Reserved for the reducer (P2+): push a caller-built change directly.
    void record_text_change(const TextChange& change);

private:
    // Splices `change` into `lines_`, marks dirty, bumps revision. P1.4 hooks
    // undo recording right here.
    void apply_change(const TextChange& change);

    std::vector<std::string> lines_ = {""}; // invariant: size() >= 1
    std::filesystem::path path_;
    bool dirty_ = false;
    std::uint64_t revision_ = 0;
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
