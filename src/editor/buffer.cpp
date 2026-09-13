#include <editor/buffer.hpp>

#include <utils/logger.hpp>

#include <format>

void Buffer::set_buffer_path(const fs::path &path)
{
    buffer_path = path;
    load();
}

void Buffer::set_save_path(const fs::path &path)
{
    buffer_path = path;
}

fs::path Buffer::get_buffer_path() const
{
    return buffer_path;
}

void Buffer::load()
{
    dirty = false;
    content.clear();

    if (buffer_path.empty())
    {
        content = {""};
        return;
    }

    const auto file_content = fs.read_file(buffer_path);
    if (!file_content.has_value())
    {
        content = {""};
        Logger::warning(std::format("Buffer load failed: {}", buffer_path.string()));
        return;
    }

    content = StrUtils::split(file_content.value(), '\n');
    if (content.empty())
        content.push_back("");

    Logger::info(std::format(
        "Buffer loaded: {} ({} lines)",
        buffer_path.string(),
        content.size()));
}

const std::vector<std::string> &Buffer::lines() const
{
    return content;
}

bool Buffer::is_dirty() const
{
    return dirty;
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

void Buffer::insert_char(int line, int column, char ch)
{
    ensure_line_exists(line);
    column = clamp_column(line, column);

    content[line].insert(static_cast<std::size_t>(column), 1, ch);
    dirty = true;
}

void Buffer::insert_newline(int line, int column)
{
    ensure_line_exists(line);
    column = clamp_column(line, column);

    std::string &text = content[line];
    const std::string tail = text.substr(static_cast<std::size_t>(column));
    text.erase(static_cast<std::size_t>(column));
    content.insert(content.begin() + line + 1, tail);
    dirty = true;
}

void Buffer::insert_empty_line(int line)
{
    if (line < 0)
        line = 0;
    if (line > static_cast<int>(content.size()))
        line = static_cast<int>(content.size());

    content.insert(content.begin() + line, "");
    dirty = true;
}

void Buffer::delete_char_before(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    std::string &text = content[line];

    if (column > 0)
    {
        text.erase(static_cast<std::size_t>(column - 1), 1);
        dirty = true;
        return;
    }

    if (line == 0)
        return;

    content[line - 1] += text;
    content.erase(content.begin() + line);
    dirty = true;
}

void Buffer::delete_char_at(int line, int column)
{
    if (content.empty() || line < 0 || line >= static_cast<int>(content.size()))
        return;

    std::string &text = content[line];

    if (column >= 0 && column < static_cast<int>(text.size()))
    {
        text.erase(static_cast<std::size_t>(column), 1);
        dirty = true;
    }
}

bool Buffer::save()
{
    if (buffer_path.empty())
    {
        Logger::warning("Buffer save skipped: empty path");
        return false;
    }

    const bool ok = fs.write_file(buffer_path, content);
    if (ok)
    {
        dirty = false;
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

    buffer_path = path;
    return save();
}
