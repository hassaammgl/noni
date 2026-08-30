#include <sidebar/dirscanner.hpp>

#include <algorithm>
#include <format>
#include <ranges>

bool DirScanner::compare_entries(const ScanedEntry &a, const ScanedEntry &b)
{
    if (a.is_dir != b.is_dir)
    {
        return a.is_dir > b.is_dir;
    }
    return a.entry_path.filename().string() < b.entry_path.filename().string();
}

DirScanner::DirScanner(const fs::path &project_path)
{
    this->project_path = project_path;
}

void DirScanner::set_project_path(const fs::path &project_path)
{
    this->project_path = project_path;
    Logger::debug(std::format("DirScanner project path: {}", project_path.string()));
}

void DirScanner::scan_dirs()
{
    fs_entries.clear();
    Logger::debug(std::format("Scanning directory: {}", project_path.string()));

    std::vector<fs::path> dirs = fs.list_directory(project_path);
    if (dirs.empty())
    {
        Logger::debug(std::format("Directory empty or unreadable: {}", project_path.string()));
        return;
    }

    for (const fs::path &entry : dirs)
    {
        if (entry.filename() == ".git")
        {
            continue;
        }
        ScanedEntry scanned_entry = check_entry(entry);
        fs_entries.push_back(std::move(scanned_entry));
    }
    std::ranges::sort(fs_entries, compare_entries);
    Logger::info(std::format(
        "Directory scan complete: {} ({} entries)",
        project_path.string(),
        fs_entries.size()));
}

ScanedEntry DirScanner::check_entry(const fs::path &entry_path)
{
    ScanedEntry se;

    se.entry_path = entry_path;
    se.is_dir = fs.is_directory(entry_path);
    se.is_file = fs.is_file(entry_path);

    if (se.is_file)
    {
        se.is_empty = true;
        return se;
    }

    if (!se.is_dir)
    {
        se.is_empty = true;
        return se;
    }

    const auto dirs = fs.list_directory(entry_path);
    std::vector<fs::path> filtered_entries;

    for (const auto &entry : dirs)
    {
        if (entry.filename() == ".git")
        {
            continue;
        }
        filtered_entries.push_back(entry);
    }

    se.is_empty = filtered_entries.empty();

    for (const fs::path &entry : filtered_entries)
    {
        se.inner_entries.push_back(check_entry(entry));
    }
    std::ranges::sort(se.inner_entries, compare_entries);
    return se;
}

const std::vector<ScanedEntry> &DirScanner::get_entries() const
{
    return fs_entries;
}
