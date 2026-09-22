#include <sidebar/dirscanner.hpp>
#include <utils/async.hpp>

#include <algorithm>
#include <format>
#include <ranges>

#include "dirscanner_detail.hpp"

using namespace dirscanner_detail;

void DirScanner::scan_dirs()
{
    fs::path root;
    {
        std::lock_guard lock(mu);
        root = project_path;
    }

    std::vector<ScanedEntry> next;
    if (!root.empty() && fs.exists(root) && fs.is_directory(root))
    {
        Logger::debug(std::format("Scanning directory (lazy): {}", root.string()));
        for (const fs::path &entry : fs.list_directory(root))
        {
            if (should_skip(entry.filename()))
                continue;
            next.push_back(make_entry(entry, false));
        }
        std::ranges::sort(next, compare_entries);
    }
    else
    {
        Logger::warning(std::format("Directory missing: {}", root.string()));
    }

    {
        std::lock_guard lock(mu);
        if (project_path != root)
            return;
        fs_entries = std::move(next);
    }
    scanning.store(false, std::memory_order_release);
    ver.fetch_add(1, std::memory_order_relaxed);
    Logger::info(std::format(
        "Directory scan complete: {} ({} entries)",
        root.string(),
        get_entries().size()));
}

void DirScanner::scan_dirs_async()
{
    fs::path root;
    {
        std::lock_guard lock(mu);
        root = project_path;
    }
    if (root.empty())
        return;

    const std::uint64_t token = Background::instance().next_token();
    job_token.store(token, std::memory_order_relaxed);
    scanning.store(true, std::memory_order_release);

    Background::instance().post([this, root, token]() {
        if (job_token.load(std::memory_order_relaxed) != token)
            return;

        std::vector<ScanedEntry> next;
        FS local_fs;
        if (!root.empty() && local_fs.exists(root) && local_fs.is_directory(root))
        {
            for (const fs::path &entry : local_fs.list_directory(root))
            {
                if (should_skip(entry.filename()))
                    continue;

                ScanedEntry se;
                se.entry_path = entry;
                se.is_dir = local_fs.is_directory(entry);
                se.is_file = local_fs.is_file(entry);
                if (se.is_dir)
                {
                    bool any = false;
                    for (const auto &child : local_fs.list_directory(entry))
                    {
                        if (should_skip(child.filename()))
                            continue;
                        any = true;
                        break;
                    }
                    se.is_empty = !any;
                    se.children_loaded = false;
                }
                else
                {
                    se.is_empty = true;
                    se.children_loaded = true;
                }
                next.push_back(std::move(se));
            }
            std::ranges::sort(next, compare_entries);
        }

        if (job_token.load(std::memory_order_relaxed) != token)
            return;

        {
            std::lock_guard lock(mu);
            if (project_path != root)
                return;
            fs_entries = std::move(next);
        }
        scanning.store(false, std::memory_order_release);
        ver.fetch_add(1, std::memory_order_relaxed);
        Logger::info(std::format(
            "Directory scan(async): {} ({} entries)",
            root.string(),
            get_entries().size()));
    });
}

bool DirScanner::ensure_loaded(const fs::path &dir_path)
{
    std::lock_guard lock(mu);
    ScanedEntry *entry = find_entry(fs_entries, dir_path);
    if (!entry || !entry->is_dir)
        return false;
    if (entry->children_loaded)
        return false;
    load_children(*entry);
    ver.fetch_add(1, std::memory_order_relaxed);
    return true;
}

bool DirScanner::is_scanning() const
{
    return scanning.load(std::memory_order_acquire);
}

std::uint64_t DirScanner::version() const
{
    return ver.load(std::memory_order_acquire);
}

std::vector<ScanedEntry> DirScanner::get_entries() const
{
    std::lock_guard lock(mu);
    return fs_entries;
}
