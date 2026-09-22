#include "buffer_detail.hpp"

using namespace buffer_detail;

UndoResult Buffer::undo()
{
    UndoResult result;
    if (undo_stack.empty())
        return result;

    UndoTransaction txn = std::move(undo_stack.back());
    undo_stack.pop_back();
    apply_transaction_reverse(txn);
    content_id = txn.id_before;
    bump_revision();
    syntax_engine_.invalidate_all();
    redo_stack.push_back(std::move(txn));

    result.ok = true;
    result.cursor_line = redo_stack.back().cursor_before_line;
    result.cursor_col = redo_stack.back().cursor_before_col;
    if (reload_listener_)
        reload_listener_(*this);
    return result;
}

UndoResult Buffer::redo()
{
    UndoResult result;
    if (redo_stack.empty())
        return result;

    UndoTransaction txn = std::move(redo_stack.back());
    redo_stack.pop_back();
    apply_transaction_forward(txn);
    content_id = txn.id_after;
    bump_revision();
    syntax_engine_.invalidate_all();
    undo_stack.push_back(std::move(txn));

    result.ok = true;
    result.cursor_line = undo_stack.back().cursor_after_line;
    result.cursor_col = undo_stack.back().cursor_after_col;
    if (reload_listener_)
        reload_listener_(*this);
    return result;
}

bool Buffer::can_undo() const
{
    return !undo_stack.empty() || (edit_active && !open_edit.changes.empty());
}

bool Buffer::can_redo() const
{
    return !redo_stack.empty();
}

bool Buffer::save()
{
    if (buffer_path.empty())
    {
        Logger::warning("Buffer save skipped: empty path");
        return false;
    }

    if (load_state == BufferLoadState::LoadError)
    {
        Logger::error(std::format(
            "Buffer save blocked (load error): {} ({})",
            buffer_path.string(),
            load_error));
        return false;
    }

    if (edit_active)
        end_edit(open_edit.cursor_before_line, open_edit.cursor_before_col);

    const bool ok = fs.write_file(buffer_path, content);
    if (ok)
    {
        saved_content_id = content_id;
        load_state = BufferLoadState::Loaded;
        load_error.clear();
        capture_disk_mtime();
        Logger::info(std::format("Buffer saved: {}", buffer_path.string()));
    }
    else
    {
        Logger::error(std::format("Buffer save failed: {}", buffer_path.string()));
    }

    return ok;
}

bool Buffer::save_as(const fs::path &path)
{
    if (path.empty())
        return save();

    if (edit_active)
        end_edit(open_edit.cursor_before_line, open_edit.cursor_before_col);

    const bool ok = fs.write_file(path, content);
    if (!ok)
    {
        Logger::error(std::format("Buffer save-as failed: {}", path.string()));
        return false;
    }

    buffer_path = path;
    saved_content_id = content_id;
    load_state = BufferLoadState::Loaded;
    load_error.clear();
    capture_disk_mtime();
    Logger::info(std::format("Buffer saved as: {}", path.string()));
    return true;
}

void Buffer::apply_recovered_content(std::vector<std::string> lines)
{
    if (edit_active)
        end_edit(open_edit.cursor_before_line, open_edit.cursor_before_col);

    content = std::move(lines);
    if (content.empty())
        content.push_back("");

    // Keep saved_content_id from last disk load/save so buffer stays dirty
    // until the user explicitly saves (never auto-overwrite source files).
    content_id = next_content_id++;
    load_state = BufferLoadState::Loaded;
    load_error.clear();
    clear_history();
    bump_revision();
    sync_syntax();
    if (reload_listener_)
        reload_listener_(*this);
    Logger::info(std::format(
        "Buffer recovered from snapshot: {} ({} lines)",
        buffer_path.empty() ? "(untitled)" : buffer_path.string(),
        content.size()));
}
