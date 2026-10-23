#include <sidebar/dirscanner.hpp>
#include <utils/async.hpp>

#include <algorithm>
#include <format>
#include <ranges>

#include "dirscanner_detail.hpp"

using namespace dirscanner_detail;

bool DirScanner::compare_entries(const ScanedEntry &a, const ScanedEntry &b)
{
    if (a.is_dir != b.is_dir)
        return a.is_dir > b.is_dir;
    return a.entry_path.filename().string() < b.entry_path.filename().string();
}

DirScanner::DirScanner(const fs::path &project_path)
{
    this->project_path = project_path;
}

void DirScanner::set_project_path(const fs::path &project_path)
{
    std::lock_guard lock(mu);
    this->project_path = project_path;
    fs_entries.clear();
    ver.fetch_add(1, std::memory_order_relaxed);
    Logger::debug(std::format("DirScanner project path: {}", project_path.string()));
}

ScanedEntry DirScanner::make_entry(const fs::path &entry_path, bool /*load_children_now*/)
{
    ScanedEntry se;
    se.entry_path = entry_path;
    se.is_dir = fs.is_directory(entry_path);
    se.is_file = fs.is_file(entry_path);

    if (se.is_file || !se.is_dir)
    {
        se.is_empty = true;
        se.children_loaded = true;
        return se;
    }

    const auto dirs = fs.list_directory(entry_path);
    bool any = false;
    for (const auto &entry : dirs)
    {
        if (should_skip(entry.filename()))
            continue;
        any = true;
        break;
    }
    se.is_empty = !any;
    se.children_loaded = false;
    return se;
}

void DirScanner::load_children(ScanedEntry &entry)
{
    entry.inner_entries.clear();
    if (!entry.is_dir)
    {
        entry.children_loaded = true;
        entry.is_empty = true;
        return;
    }

    const auto dirs = fs.list_directory(entry.entry_path);
    for (const auto &child : dirs)
    {
        if (should_skip(child.filename()))
            continue;
        // Nested dirs stay lazy.
        entry.inner_entries.push_back(make_entry(child, false));
    }
    std::ranges::sort(entry.inner_entries, compare_entries);
    entry.is_empty = entry.inner_entries.empty();
    entry.children_loaded = true;
}

ScanedEntry *DirScanner::find_entry(std::vector<ScanedEntry> &entries, const fs::path &path)
{
    const std::string want = path_key(path);
    for (auto &e : entries)
    {
        if (path_key(e.entry_path) == want)
            return &e;
        if (e.is_dir && e.children_loaded)
        {
            if (auto *found = find_entry(e.inner_entries, path))
                return found;
        }
    }
    return nullptr;
}

