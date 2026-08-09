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
    bool write_file(const fs::path &path, const std::string &content);
    bool append_file(const fs::path &path, const std::string &content);
    std::optional<std::string> read_file(const fs::path &path);
    bool delete_file(const fs::path &path);
    bool rename_file(const fs::path &oldfile, const fs::path &newpath);
    bool copy_file(const fs::path &from, const fs::path &to);
    // directory operations
    bool createDirectory(const fs::path &path);
    bool deleteDirectory(const fs::path &path);
    std::vector<fs::path> listDirectory(const fs::path &path);
    // info
    bool exists(const fs::path &path) const;
    bool is_file(const fs::path &path) const;
    bool is_directory(const fs::path &path) const;
    uintmax_t file_size(const fs::path &path) const;
    // paths
    fs::path currentPath() const;
    bool changeCurrentPath(const fs::path &path);
    fs::path absolute(const fs::path &path) const;
    fs::path canonical(const fs::path &path) const;
    fs::path weaklyCanonical(const fs::path &path) const;
};
