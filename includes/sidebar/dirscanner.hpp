#pragma once

#include "utils/fs.hpp"
#include "utils/logger.hpp"

#include <filesystem>
#include <vector>

struct ScanedEntry
{
    fs::path entriePath;

    bool isDir = false;
    bool isFile = false;
    bool isEmpty = true;
    std::vector<ScanedEntry> innerEntries;
};

class DirScanner
{
private:
    FS fs;
    Logger l;

    fs::path projectPath;
    std::vector<ScanedEntry> fsentries;
    static bool compareEntries(const ScanedEntry &a, const ScanedEntry &b);

public:
    DirScanner() = default;
    explicit DirScanner(const fs::path &projectPath);
    ~DirScanner() = default;

    void setProjectPath(const fs::path &projectPath);

    void scanDirs();

    ScanedEntry checkEntrie(const fs::path &entriePath);

    const std::vector<ScanedEntry> &getEntries() const;
};