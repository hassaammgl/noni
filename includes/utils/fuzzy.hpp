#pragma once

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

struct FuzzyMatch
{
    fs::path path;
    std::string display;
    int score = 0;
};

namespace Fuzzy
{
    int score(std::string_view text, std::string_view query);

    std::vector<FuzzyMatch> filter(
        const std::vector<fs::path> &files,
        const fs::path &root,
        std::string_view query,
        std::size_t limit = 200);
}

class FileIndex
{
public:
    void set_root(const fs::path &root);
    void rebuild();       // sync
    void rebuild_async(); // background; UI should poll ready()
    bool is_indexing() const;
    std::uint64_t version() const;

    // Thread-safe snapshot for filtering / drawing.
    fs::path root() const;
    std::vector<fs::path> files() const;
    std::size_t file_count() const;

private:
    mutable std::mutex mu;
    fs::path root_path;
    std::vector<fs::path> entries;
    std::atomic<bool> indexing{false};
    std::atomic<std::uint64_t> ver{0};
    std::atomic<std::uint64_t> job_token{0};

    static bool should_skip_dir(const fs::path &name);
    static std::vector<fs::path> scan_files(const fs::path &root);
};
