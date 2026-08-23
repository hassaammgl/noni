#include "sidebar/dirscanner.hpp"

#include <algorithm>
#include <ranges>
#include <array>
#include <functional>
#include <iomanip>
#include <iostream>

bool DirScanner::compareEntries(const ScanedEntry &a, const ScanedEntry &b)
{
    if (a.isDir != b.isDir)
    {
        return a.isDir > b.isDir;
    }
    return a.entriePath.filename().string() < b.entriePath.filename().string();
}

DirScanner::DirScanner(const fs::path &projectPath)
{
    this->projectPath = projectPath;
}
void DirScanner::setProjectPath(const fs::path &projectPath)
{
    this->projectPath = projectPath;
}

void DirScanner::scanDirs()
{
    fsentries.clear();
    std::vector<fs::path> dirs = fs.listDirectory(projectPath);
    if (dirs.empty())
    {
        return;
    }

    for (const fs::path &entry : dirs)
    {
        if (entry.filename() == ".git")
        {
            continue;
        }
        ScanedEntry scannedEntry = checkEntrie(entry);
        fsentries.push_back(std::move(scannedEntry));
    }
    std::ranges::sort(fsentries, compareEntries);
}

ScanedEntry DirScanner::checkEntrie(const fs::path &entriePath)
{
    ScanedEntry se;

    se.entriePath = entriePath;
    se.isDir = fs.is_directory(entriePath);
    se.isFile = fs.is_file(entriePath);

    if (se.isFile)
    {
        se.isEmpty = true;
        return se;
    }

    if (!se.isDir)
    {
        se.isEmpty = true;
        return se;
    }

    const auto dirs = fs.listDirectory(entriePath);
    std::vector<fs::path> filteredEntries;

    for (const auto &entry : dirs)
    {
        if (entry.filename() == ".git")
        {
            continue;
        }
        filteredEntries.push_back(entry);
    }

    se.isEmpty = filteredEntries.empty();

    for (const fs::path &entry : filteredEntries)
    {
        se.innerEntries.push_back(checkEntrie(entry));
    }
    std::ranges::sort(se.innerEntries, compareEntries);
    return se;
}

const std::vector<ScanedEntry> &DirScanner::getEntries() const
{
    return fsentries;
}