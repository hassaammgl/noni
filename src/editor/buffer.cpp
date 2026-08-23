#include <editor/buffer.hpp>

void Buffer::setBufferPath(const fs::path &bufferPath)
{
    this->bufferPath = bufferPath;
}

fs::path Buffer::getBufferPath()
{
    return this->bufferPath;
}

std::vector<std::string> Buffer::readBuffer()
{
    std::string con = fs.read_file(this->bufferPath).value_or("");
    auto lines = StrUtils::split(con, '\n');
    return lines;
}

bool Buffer::writeBuffer(std::vector<std::string> &content)
{
    return fs.write_file(this->bufferPath, content);
}

bool Buffer::deleteBuffer(const fs::path &bufferPath)
{
    return fs.delete_file(bufferPath);
}
