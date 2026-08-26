#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class FS
{
public:
    // file operations
    bool create_file(const fs::path &path);
    bool write_file(const fs::path &path, const std::vector<std::string> &content);
    bool append_file(const fs::path &path, const std::string &content);
    std::optional<std::string> read_file(const fs::path &path);
    bool delete_file(const fs::path &path);
    bool rename_file(const fs::path &oldfile, const fs::path &newpath);
    bool copy_file(const fs::path &from, const fs::path &to);
    // directory operations
    bool create_directory(const fs::path &path);
    bool delete_directory(const fs::path &path);
    std::vector<fs::path> list_directory(const fs::path &path);
    // info
    bool exists(const fs::path &path) const;
    bool is_file(const fs::path &path) const;
    bool is_directory(const fs::path &path) const;
    uintmax_t file_size(const fs::path &path) const;
    // paths
    fs::path current_path() const;
    bool change_current_path(const fs::path &path);
    fs::path absolute(const fs::path &path) const;
    fs::path canonical(const fs::path &path) const;
    fs::path weakly_canonical(const fs::path &path) const;
};