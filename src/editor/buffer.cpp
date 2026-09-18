#include <editor/buffer.hpp>

#include <utils/logger.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <format>
#include <system_error>

namespace
{
    void advance_pos(
        int &line,
        int &col,
        std::string_view text,
        const std::vector<std::string> & /*lines*/)
    {
        for (char ch : text)
        {
            if (ch == '\n')
            {
                ++line;
                col = 0;
            }
            else
            {
                ++col;
            }
        }
    }
}

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

void Buffer::begin_edit(int cursor_line, int cursor_col)
{
    if (edit_active)
        end_edit(cursor_line, cursor_col);

    edit_active = true;
    open_edit = {};
    open_edit.cursor_before_line = cursor_line;
    open_edit.cursor_before_col = cursor_col;
    open_edit.id_before = content_id;
}

void Buffer::end_edit(int cursor_line, int cursor_col)
{
    if (!edit_active)
        return;

    edit_active = false;
    if (open_edit.changes.empty())
    {
        open_edit = {};
        return;
    }

    open_edit.cursor_after_line = cursor_line;
    open_edit.cursor_after_col = cursor_col;
    open_edit.id_after = next_content_id++;
    content_id = open_edit.id_after;

    undo_stack.push_back(std::move(open_edit));
    open_edit = {};
    redo_stack.clear();
    trim_undo_stack();
}

bool Buffer::is_edit_active() const
{
    return edit_active;
}

void Buffer::trim_undo_stack()
{
    while (undo_stack.size() > kMaxUndo)
        undo_stack.erase(undo_stack.begin());
}

void Buffer::raw_insert_char(int line, int column, char ch)
{
    ensure_line_exists(line);
    column = clamp_column(line, column);
    content[static_cast<std::size_t>(line)].insert(
        static_cast<std::size_t>(column), 1, ch);
}

std::pair<int, int> Buffer::raw_insert_text(int line, int column, std::string_view text)
{
    ensure_line_exists(line);
    column = clamp_column(line, column);

    if (text.empty())
        return {line, column};

    std::vector<std::string> parts;
    std::string cur;
    parts.reserve(8);

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '\r')
        {
            if (i + 1 < text.size() && text[i + 1] == '\n')
                continue;
            parts.push_back(std::move(cur));
            cur.clear();
            continue;
        }
        if (c == '\n')
        {
            parts.push_back(std::move(cur));
            cur.clear();
            continue;
        }
        if (c == '\t' || c >= 32)
            cur.push_back(static_cast<char>(c));
    }
    parts.push_back(std::move(cur));

    if (parts.size() == 1)
    {
        content[static_cast<std::size_t>(line)].insert(
            static_cast<std::size_t>(column),
            parts[0]);
        return {line, column + static_cast<int>(parts[0].size())};
    }

    std::string &first = content[static_cast<std::size_t>(line)];
    const std::string tail = first.substr(static_cast<std::size_t>(column));
    first.erase(static_cast<std::size_t>(column));
    first += parts[0];

    for (std::size_t i = 1; i + 1 < parts.size(); ++i)
    {
        content.insert(
            content.begin() + static_cast<std::ptrdiff_t>(line) + static_cast<std::ptrdiff_t>(i),
            parts[i]);
    }

    const std::size_t last_idx = static_cast<std::size_t>(line) + parts.size() - 1;
    content.insert(
        content.begin() + static_cast<std::ptrdiff_t>(last_idx),
        parts.back() + tail);

    return {
        static_cast<int>(last_idx),
        static_cast<int>(parts.back().size())};
}

void Buffer::raw_insert_newline(int line, int column)
{
    ensure_line_exists(line);
    column = clamp_column(line, column);
    std::string &text = content[static_cast<std::size_t>(line)];
    const std::string tail = text.substr(static_cast<std::size_t>(column));
    text.erase(static_cast<std::size_t>(column));
    content.insert(content.begin() + line + 1, tail);
}

void Buffer::raw_insert_empty_line(int line)
{
    if (line < 0)
        line = 0;
    if (line > static_cast<int>(content.size()))
        line = static_cast<int>(content.size());
    content.insert(content.begin() + line, "");
}

void Buffer::raw_delete_char_before(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    std::string &text = content[static_cast<std::size_t>(line)];
    if (column > 0)
    {
        const std::size_t end = static_cast<std::size_t>(std::min(column, static_cast<int>(text.size())));
        const std::size_t start = TextMetrics::prev_cp(text, end);
        text.erase(start, end - start);
        return;
    }
    if (line == 0)
        return;

    content[static_cast<std::size_t>(line - 1)] += text;
    content.erase(content.begin() + line);
}

void Buffer::raw_delete_char_at(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    std::string &text = content[static_cast<std::size_t>(line)];
    if (column < 0 || column >= static_cast<int>(text.size()))
        return;
    const std::size_t start = TextMetrics::snap_byte(text, static_cast<std::size_t>(column));
    const std::size_t end = TextMetrics::next_cp(text, start);
    text.erase(start, end - start);
}

void Buffer::raw_delete_text(int line, int column, std::string_view text)
{
    int l = line;
    int c = column;
    for (char ch : text)
    {
        if (ch == '\n')
        {
            if (l < 0 || l + 1 >= static_cast<int>(content.size()))
                return;
            if (c != static_cast<int>(content[static_cast<std::size_t>(l)].size()))
                return;
            content[static_cast<std::size_t>(l)] += content[static_cast<std::size_t>(l + 1)];
            content.erase(content.begin() + l + 1);
            continue;
        }

        if (l < 0 || l >= static_cast<int>(content.size()))
            return;
        auto &row = content[static_cast<std::size_t>(l)];
        if (c < 0 || c >= static_cast<int>(row.size()) || row[static_cast<std::size_t>(c)] != ch)
            return;
        row.erase(static_cast<std::size_t>(c), 1);
    }
}

void Buffer::apply_forward(const TextChange &change)
{
    if (!change.deleted.empty())
        raw_delete_text(change.start_line, change.start_col, change.deleted);
    if (!change.inserted.empty())
        (void)raw_insert_text(change.start_line, change.start_col, change.inserted);
}

void Buffer::apply_reverse(const TextChange &change)
{
    TextChange inv = change;
    std::swap(inv.deleted, inv.inserted);
    apply_forward(inv);
}

void Buffer::apply_transaction_forward(const UndoTransaction &txn)
{
    for (const auto &c : txn.changes)
        apply_forward(c);
}

void Buffer::apply_transaction_reverse(const UndoTransaction &txn)
{
    for (auto it = txn.changes.rbegin(); it != txn.changes.rend(); ++it)
        apply_reverse(*it);
}

void Buffer::record_change(TextChange change)
{
    const bool auto_wrap = !edit_active;
    if (auto_wrap)
        begin_edit(change.start_line, change.start_col);

    // Coalesce contiguous pure inserts into the last change.
    if (!open_edit.changes.empty() && change.deleted.empty() && !change.inserted.empty())
    {
        TextChange &last = open_edit.changes.back();
        if (last.deleted.empty() && !last.inserted.empty())
        {
            int el = last.start_line;
            int ec = last.start_col;
            advance_pos(el, ec, last.inserted, content);
            if (el == change.start_line && ec == change.start_col)
            {
                last.inserted += change.inserted;
                apply_forward(change);
                syntax_engine_.notify_edit(change);
                bump_revision();
                if (change_listener_)
                    change_listener_(*this, change);
                if (auto_wrap)
                    end_edit(change.start_line, change.start_col);
                return;
            }
        }
    }

    // Coalesce repeated deletes at the same start (e.g. S clearing a line).
    if (!open_edit.changes.empty() && change.inserted.empty() && !change.deleted.empty())
    {
        TextChange &last = open_edit.changes.back();
        if (last.inserted.empty() && !last.deleted.empty() &&
            last.start_line == change.start_line && last.start_col == change.start_col)
        {
            last.deleted += change.deleted;
            apply_forward(change);
            syntax_engine_.notify_edit(change);
            bump_revision();
            if (change_listener_)
                change_listener_(*this, change);
            if (auto_wrap)
                end_edit(change.start_line, change.start_col);
            return;
        }
    }

    apply_forward(change);
    syntax_engine_.notify_edit(change);
    if (change_listener_)
        change_listener_(*this, change);
    open_edit.changes.push_back(std::move(change));
    bump_revision();

    if (auto_wrap)
    {
        // Cursor after unknown — caller should use begin/end; for auto use start.
        end_edit(change.start_line, change.start_col);
    }
}

void Buffer::insert_char(int line, int column, char ch)
{
    TextChange change;
    change.start_line = line;
    change.start_col = column;
    change.inserted.assign(1, ch);
    record_change(std::move(change));
}

std::pair<int, int> Buffer::insert_text(int line, int column, std::string_view text)
{
    if (text.empty())
        return {line, column};

    // Normalize to the same filtering as raw_insert_text for the recorded string.
    std::string filtered;
    filtered.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '\r')
        {
            if (i + 1 < text.size() && text[i + 1] == '\n')
                continue;
            filtered.push_back('\n');
            continue;
        }
        if (c == '\n')
        {
            filtered.push_back('\n');
            continue;
        }
        if (c == '\t' || c >= 32)
            filtered.push_back(static_cast<char>(c));
    }

    if (filtered.empty())
        return {line, column};

    TextChange change;
    change.start_line = line;
    change.start_col = column;
    change.inserted = filtered;

    const bool auto_wrap = !edit_active;
    if (auto_wrap)
        begin_edit(line, column);

    apply_forward(change);
    syntax_engine_.notify_edit(change);
    if (change_listener_)
        change_listener_(*this, change);
    open_edit.changes.push_back(std::move(change));
    bump_revision();

    int el = line;
    int ec = column;
    advance_pos(el, ec, filtered, content);

    if (auto_wrap)
        end_edit(el, ec);

    return {el, ec};
}

void Buffer::insert_newline(int line, int column)
{
    TextChange change;
    change.start_line = line;
    change.start_col = column;
    change.inserted = "\n";
    record_change(std::move(change));
}

void Buffer::insert_empty_line(int line)
{
    // Inserting "" at index `line` == inserting '\n' at beginning of that line
    // (or at end of document).
    TextChange change;
    change.start_line = line;
    change.start_col = 0;
    if (line >= static_cast<int>(content.size()))
    {
        // Append: put newline at end of last line.
        if (content.empty())
        {
            content.push_back("");
        }
        change.start_line = static_cast<int>(content.size()) - 1;
        change.start_col = static_cast<int>(content.back().size());
    }
    change.inserted = "\n";
    record_change(std::move(change));
}

void Buffer::delete_char_before(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    TextChange change;
    if (column > 0)
    {
        const auto &row = content[static_cast<std::size_t>(line)];
        const std::size_t end = static_cast<std::size_t>(std::min(column, static_cast<int>(row.size())));
        const std::size_t start = TextMetrics::prev_cp(row, end);
        change.start_line = line;
        change.start_col = static_cast<int>(start);
        change.deleted = row.substr(start, end - start);
    }
    else
    {
        if (line == 0)
            return;
        change.start_line = line - 1;
        change.start_col = static_cast<int>(content[static_cast<std::size_t>(line - 1)].size());
        change.deleted = "\n";
    }
    record_change(std::move(change));
}

void Buffer::delete_char_at(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    const auto &row = content[static_cast<std::size_t>(line)];
    if (column < 0 || column >= static_cast<int>(row.size()))
        return;

    const std::size_t start = TextMetrics::snap_byte(row, static_cast<std::size_t>(column));
    const std::size_t end = TextMetrics::next_cp(row, start);
    TextChange change;
    change.start_line = line;
    change.start_col = static_cast<int>(start);
    change.deleted = row.substr(start, end - start);
    record_change(std::move(change));
}

std::string Buffer::get_range_text(int start_line, int start_col, int end_line, int end_col) const
{
    if (content.empty())
        return {};

    if (start_line < 0)
        start_line = 0;
    if (start_col < 0)
        start_col = 0;

    if (end_line > static_cast<int>(content.size()))
        end_line = static_cast<int>(content.size());
    if (end_line == static_cast<int>(content.size()))
        end_col = 0;

    if (start_line >= static_cast<int>(content.size()))
        return {};

    if (start_line > end_line)
        return {};
    if (start_line == end_line && start_col >= end_col)
        return {};

    start_col = clamp_column(start_line, start_col);

    std::string out;

    // Through EOF (linewise last lines): no trailing newline after final line.
    if (end_line >= static_cast<int>(content.size()))
    {
        out.append(content[static_cast<std::size_t>(start_line)].substr(
            static_cast<std::size_t>(start_col)));
        for (int line = start_line + 1; line < static_cast<int>(content.size()); ++line)
        {
            out.push_back('\n');
            out.append(content[static_cast<std::size_t>(line)]);
        }
        return out;
    }

    end_col = clamp_column(end_line, end_col);

    if (start_line == end_line)
    {
        const auto &row = content[static_cast<std::size_t>(start_line)];
        return row.substr(
            static_cast<std::size_t>(start_col),
            static_cast<std::size_t>(end_col - start_col));
    }

    out.append(content[static_cast<std::size_t>(start_line)].substr(
        static_cast<std::size_t>(start_col)));
    out.push_back('\n');

    for (int line = start_line + 1; line < end_line; ++line)
    {
        out.append(content[static_cast<std::size_t>(line)]);
        out.push_back('\n');
    }

    out.append(content[static_cast<std::size_t>(end_line)].substr(
        0, static_cast<std::size_t>(end_col)));
    return out;
}

void Buffer::delete_range(int start_line, int start_col, int end_line, int end_col)
{
    const std::string deleted = get_range_text(start_line, start_col, end_line, end_col);
    if (deleted.empty())
        return;

    TextChange change;
    change.start_line = start_line;
    change.start_col = start_col;
    change.deleted = deleted;
    record_change(std::move(change));
}

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
