#include "buffer_detail.hpp"

using namespace buffer_detail;

void Buffer::capture_disk_mtime()
{
    disk_mtime_.reset();
    external_change_notified_ = false;
    if (buffer_path.empty())
        return;
    std::error_code ec;
    const auto mt = fs::last_write_time(buffer_path, ec);
    if (!ec)
        disk_mtime_ = mt;
}

bool Buffer::disk_changed() const
{
    if (buffer_path.empty() || !disk_mtime_)
        return false;
    std::error_code ec;
    const auto mt = fs::last_write_time(buffer_path, ec);
    if (ec)
        return false;
    return mt != *disk_mtime_;
}

void Buffer::clear_external_change_flag()
{
    external_change_notified_ = false;
}

void Buffer::set_buffer_path(const fs::path &path)
{
    buffer_path = path;
    load();
}

void Buffer::set_save_path(const fs::path &path)
{
    buffer_path = path;
    capture_disk_mtime();
}

fs::path Buffer::get_buffer_path() const
{
    return buffer_path;
}

void Buffer::clear_history()
{
    undo_stack.clear();
    redo_stack.clear();
    edit_active = false;
    open_edit = {};
}

void Buffer::load()
{
    ++revision;
    content.clear();
    load_error.clear();
    clear_history();
    content_id = 0;
    saved_content_id = 0;
    next_content_id = 1;
    syntax_engine_.invalidate_all();

    if (buffer_path.empty())
    {
        content = {""};
        load_state = BufferLoadState::Loaded;
        disk_mtime_.reset();
        external_change_notified_ = false;
        if (reload_listener_)
            reload_listener_(*this);
        return;
    }

    const FsReadResult result = fs.read_file_detailed(buffer_path);
    if (!result.ok)
    {
        content = {""};
        load_state = BufferLoadState::LoadError;
        load_error = result.error.empty() ? "read failed" : result.error;
        disk_mtime_.reset();
        external_change_notified_ = false;
        Logger::warning(std::format(
            "Buffer load failed: {} ({})",
            buffer_path.string(),
            load_error));
        return;
    }

    content = StrUtils::split(result.content, '\n');
    if (content.empty())
        content.push_back("");

    load_state = BufferLoadState::Loaded;
    load_error.clear();
    capture_disk_mtime();

    Logger::info(std::format(
        "Buffer loaded: {} ({} lines)",
        buffer_path.string(),
        content.size()));
    if (reload_listener_)
        reload_listener_(*this);
}

const std::vector<std::string> &Buffer::lines() const
{
    return content;
}

bool Buffer::is_dirty() const
{
    if (content_id != saved_content_id)
        return true;
    if (edit_active && !open_edit.changes.empty())
        return true;
    return false;
}

std::uint64_t Buffer::get_revision() const
{
    return revision;
}

BufferLoadState Buffer::get_load_state() const
{
    return load_state;
}

bool Buffer::has_load_error() const
{
    return load_state == BufferLoadState::LoadError;
}

const std::string &Buffer::get_load_error() const
{
    return load_error;
}

bool Buffer::can_save() const
{
    return load_state == BufferLoadState::Loaded && !buffer_path.empty();
}

void Buffer::bump_revision()
{
    ++revision;
}

void Buffer::sync_syntax()
{
    syntax_engine_.sync(buffer_path, content, revision);
}

void Buffer::ensure_line_exists(int line)
{
    while (static_cast<int>(content.size()) <= line)
        content.push_back("");
}

int Buffer::clamp_column(int line, int column) const
{
    if (line < 0 || line >= static_cast<int>(content.size()))
        return 0;

    const int max_column = static_cast<int>(content[line].size());
    if (column < 0)
        return 0;
    if (column > max_column)
        return max_column;

    return column;
}
