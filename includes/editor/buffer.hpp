#pragma once
#include <utils/fs.hpp>
#include <utils/str.hpp>
#include <vector>
#include <string>

class Buffer
{
private:
    fs::path buffer_path;
    std::vector<std::string> content;
    FS fs;

public:
    Buffer() = default;
    ~Buffer() = default;
    void set_buffer_path(const fs::path &project_path);
    fs::path get_buffer_path();
    std::vector<std::string> read_buffer();
    bool write_buffer(std::vector<std::string> &);
    bool delete_buffer(const fs::path &buffer_path);
};