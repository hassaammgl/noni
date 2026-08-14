#include "sidebar/dirscanner.hpp"

#include <iostream>

DirScanner::DirScanner(const fs::path &projectPath)
{
    this->projectPath = projectPath;
}

DirScanner::~DirScanner()
{
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

    // Not a directory
    if (!se.isDir)
    {
        se.isEmpty = true;

        return se;
    }

    // Directory
    std::vector<fs::path> dirs = fs.listDirectory(entriePath);

    // Remove .git from children
    std::vector<fs::path> filteredEntries;

    for (const fs::path &entry : dirs)
    {
        if (entry.filename() == ".git")
        {
            continue;
        }

        filteredEntries.push_back(entry);
    }

    se.isEmpty = filteredEntries.empty();

    // Recursively scan children
    for (const fs::path &entry : filteredEntries)
    {
        se.innerEntries.push_back(checkEntrie(entry));
    }

    return se;
}

const std::vector<ScanedEntry> &DirScanner::getEntries() const
{
    return fsentries;
}