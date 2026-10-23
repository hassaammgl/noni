#pragma once

#include <utils/fs.hpp>
#include <utils/logger.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <vector>

struct ScanedEntry
{
    fs::path entry_path;
    bool is_dir = false;
    bool is_file = false;
    bool is_empty = true;
    bool children_loaded = false;
    std::vector<ScanedEntry> inner_entries;
};

class DirScanner
{
private:
    FS fs;
    fs::path project_path;
    std::vector<ScanedEntry> fs_entries;

    mutable std::mutex mu;
    std::atomic<bool> scanning{false};
    std::atomic<std::uint64_t> ver{0};
    std::atomic<std::uint64_t> job_token{0};

    static bool compare_entries(const ScanedEntry &a, const ScanedEntry &b);
    ScanedEntry make_entry(const fs::path &entry_path, bool load_children);
    ScanedEntry *find_entry(std::vector<ScanedEntry> &entries, const fs::path &path);
    void load_children(ScanedEntry &entry);

public:
    DirScanner() = default;
    explicit DirScanner(const fs::path &project_path);
    ~DirScanner() = default;

    void set_project_path(const fs::path &project_path);
    void scan_dirs();
    void scan_dirs_async();
    bool ensure_loaded(const fs::path &dir_path);
    bool is_scanning() const;
    std::uint64_t version() const;

    // Snapshot under lock for UI drawing.
    std::vector<ScanedEntry> get_entries() const;
};
