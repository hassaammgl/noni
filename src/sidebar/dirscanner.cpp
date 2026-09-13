#include <sidebar/dirscanner.hpp>
#include <utils/async.hpp>

#include <algorithm>
#include <format>
#include <ranges>

namespace
{
    bool should_skip(const fs::path &name)
    {
        static const char *skip[] = {
            ".git", ".hg", ".svn",
            "node_modules", "target", "build", "dist", "out",
            ".cache", ".idea", ".vscode", ".cursor",
            "__pycache__", ".next", ".nuxt", "vendor",
            "CMakeFiles", ".tox", ".venv", "venv",
        };
        const std::string n = name.string();
        for (const char *s : skip)
        {
            if (n == s)
                return true;
        }
        return false;
    }

    std::string path_key(const fs::path &path)
    {
        std::error_code ec;
        const fs::path abs = fs::weakly_canonical(path, ec);
        return ec ? path.lexically_normal().string() : abs.string();
    }
}

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
