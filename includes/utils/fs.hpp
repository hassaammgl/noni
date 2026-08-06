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
    bool create_file(const fs::path &path);                             // function for creating files
    bool write_file(const fs::path &path, const std::string content);   // function for write in files
    bool append_file(const fs::path &path, const std::string content);  // function for append files
    std::optional<std::string> read_file(const fs::path &path);         // function for read in files
    bool delete_file(const fs::path &path);                             // function for delete in files
    bool rename_file(const fs::path &oldfile, const fs::path &newpath); // function for rename files
    bool copy_file(const fs::path &from, const fs::path &to);           // function for copy files
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
    // 2. Existing file ka physical location resolve karta hai (symlinks, '.', '..')
    fs::path canonical(const fs::path &path) const;
    // 3. Un files ke liye jo abhi exist NAHI karti
    // Existing parent folders ke symlinks/'.'/'..' resolve kar leta hai.
    fs::path weaklyCanonical(const fs::path &path) const;
};
