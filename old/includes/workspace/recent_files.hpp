#pragma once

#include <filesystem>
#include <mutex>
#include <vector>

namespace fs = std::filesystem;

// In-memory MRU of opened files (workspace navigation). Bounded.
class RecentFiles
{
public:
    void touch(const fs::path &path);
    void clear();
    void set_limit(std::size_t n);
    std::size_t limit() const { return limit_; }

    // Newest first. Thread-safe snapshot.
    std::vector<fs::path> list() const;

private:
    mutable std::mutex mu_;
    std::vector<fs::path> entries_;
    std::size_t limit_ = 32;
};
