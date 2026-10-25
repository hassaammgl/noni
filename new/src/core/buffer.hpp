#pragma once
// P1.2 — buffer storage: line vector + file load/save + dirty/revision.
// Pure core: no ncurses, no app/ui headers, no exceptions (AGENTS.md).
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "core/pos.hpp"

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

private:
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
