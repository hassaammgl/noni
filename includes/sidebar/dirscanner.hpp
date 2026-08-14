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

    std::vector<ScanedEntry> fsentries;
    fs::path projectPath;

public:
    DirScanner(const fs::path &projectPath);
    ~DirScanner();

    void scanDirs();

    ScanedEntry checkEntrie(const fs::path &entriePath);

    const std::vector<ScanedEntry> &getEntries() const;
};