#pragma once

#include <editor/undo.hpp>
#include <syntax/tree_highlighter.hpp>
#include <utils/fs.hpp>
#include <utils/str.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

enum class BufferLoadState
{
    Loaded,    // successfully loaded (including valid empty file) or untitled
    LoadError, // open/read failed — must not be treated as an empty document
};

class Buffer
{
private:
    fs::path buffer_path;
    std::vector<std::string> content;
    FS fs;
    std::uint64_t revision = 1;
    BufferLoadState load_state = BufferLoadState::Loaded;
    std::string load_error;

    // Document identity for dirty tracking (not the highlighter revision alone).
    std::uint64_t content_id = 0;
    std::uint64_t saved_content_id = 0;
    std::uint64_t next_content_id = 1;

    std::vector<UndoTransaction> undo_stack;
    std::vector<UndoTransaction> redo_stack;
    bool edit_active = false;
    UndoTransaction open_edit;
    static constexpr std::size_t kMaxUndo = 256;

    // Shared across all Windows viewing this Buffer.
    TreeHighlighter syntax_engine_;

    void ensure_line_exists(int line);
    int clamp_column(int line, int column) const;
    void bump_revision();

    void clear_history();
    void record_change(TextChange change);
    void apply_forward(const TextChange &change);
    void apply_reverse(const TextChange &change);
    void apply_transaction_forward(const UndoTransaction &txn);
    void apply_transaction_reverse(const UndoTransaction &txn);
    void trim_undo_stack();

    // Mutations without undo recording.
    void raw_insert_char(int line, int column, char ch);
    std::pair<int, int> raw_insert_text(int line, int column, std::string_view text);
    void raw_insert_newline(int line, int column);
    void raw_insert_empty_line(int line);
    void raw_delete_char_before(int line, int column);
    void raw_delete_char_at(int line, int column);
    void raw_delete_text(int line, int column, std::string_view text);

public:
    Buffer() = default;
    ~Buffer() = default;

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    void set_buffer_path(const fs::path &path);
    void set_save_path(const fs::path &path);
    fs::path get_buffer_path() const;

    void load();
    const std::vector<std::string> &lines() const;
    bool is_dirty() const;
    std::uint64_t get_revision() const;

    BufferLoadState get_load_state() const;
    bool has_load_error() const;
    const std::string &get_load_error() const;
    bool can_save() const;

    TreeHighlighter &syntax() { return syntax_engine_; }
    const TreeHighlighter &syntax() const { return syntax_engine_; }

    // Sync syntax cache for rendering (idempotent / cheap when clean).
    void sync_syntax();

    // Undo transaction boundaries (cursor is Window-owned; passed in explicitly).
    void begin_edit(int cursor_line, int cursor_col);
    void end_edit(int cursor_line, int cursor_col);
    bool is_edit_active() const;

    UndoResult undo();
    UndoResult redo();
    bool can_undo() const;
    bool can_redo() const;

    void insert_char(int line, int column, char ch);
    std::pair<int, int> insert_text(int line, int column, std::string_view text);
    void insert_newline(int line, int column);
    void insert_empty_line(int line);
    void delete_char_before(int line, int column);
    void delete_char_at(int line, int column);

    // Exclusive-end range. end.line may equal lines().size() for linewise-through-EOF.
    std::string get_range_text(int start_line, int start_col, int end_line, int end_col) const;
    void delete_range(int start_line, int start_col, int end_line, int end_col);

    bool save();
    bool save_as(const fs::path &path);
};
