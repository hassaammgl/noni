#pragma once

#include <utils/fs.hpp>
#include <utils/str.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class Buffer
{
private:
    fs::path buffer_path;
    std::vector<std::string> content;
    FS fs;
    bool dirty = false;

    void ensure_line_exists(int line);
    int clamp_column(int line, int column) const;

public:
    Buffer() = default;
    ~Buffer() = default;

    void set_buffer_path(const fs::path &path);
    void set_save_path(const fs::path &path);
    fs::path get_buffer_path() const;

    void load();
    const std::vector<std::string> &lines() const;
    bool is_dirty() const;

    void insert_char(int line, int column, char ch);
    void insert_newline(int line, int column);
    void insert_empty_line(int line);
    void delete_char_before(int line, int column);
    void delete_char_at(int line, int column);

    bool save();
    bool save_as(const fs::path &path);
};
