#include <editor/buffer.hpp>

void Buffer::set_buffer_path(const fs::path &buffer_path)
{
    this->buffer_path = buffer_path;
}

fs::path Buffer::get_buffer_path()
{
    return this->buffer_path;
}

std::vector<std::string> Buffer::read_buffer()
{
    std::string con = fs.read_file(this->buffer_path).value_or("");
    auto lines = StrUtils::split(con, '\n');
    return lines;
}

bool Buffer::write_buffer(std::vector<std::string> &content)
{
    return fs.write_file(this->buffer_path, content);
}

bool Buffer::delete_buffer(const fs::path &buffer_path)
{
    return fs.delete_file(buffer_path);
}
