#pragma once

#include <utils/fs.hpp>
#include <utils/logger.hpp>

#include <filesystem>
#include <vector>

struct ScanedEntry
{
    fs::path entry_path;
    bool is_dir = false;
    bool is_file = false;
    bool is_empty = true;
    std::vector<ScanedEntry> inner_entries;
};

class DirScanner
{
private:
    FS fs;
    fs::path project_path;
    std::vector<ScanedEntry> fs_entries;
    static bool compare_entries(const ScanedEntry &a, const ScanedEntry &b);

public:
    DirScanner() = default;
    explicit DirScanner(const fs::path &project_path);
    ~DirScanner() = default;
    void set_project_path(const fs::path &project_path);
    void scan_dirs();
    ScanedEntry check_entry(const fs::path &entry_path);
    const std::vector<ScanedEntry> &get_entries() const;
};