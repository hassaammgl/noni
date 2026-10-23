#include "fuzzy_detail.hpp"

using namespace fuzzy_detail;

bool FileIndex::should_skip_dir(const fs::path &name)
{
    return is_skipped_name(name.string());
}

std::vector<fs::path> FileIndex::scan_files(const fs::path &root)
{
    std::vector<fs::path> found;
    if (root.empty() || !fs::exists(root))
        return found;

    std::error_code ec;
    const auto options = fs::directory_options::skip_permission_denied;
    for (auto it = fs::recursive_directory_iterator(root, options, ec);
         it != fs::recursive_directory_iterator();
         it.increment(ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

        const auto &entry = *it;
        if (entry.is_directory(ec))
        {
            if (should_skip_dir(entry.path().filename()))
                it.disable_recursion_pending();
            continue;
        }

        if (!entry.is_regular_file(ec))
            continue;

        found.push_back(entry.path());
    }

    std::sort(found.begin(), found.end());
    return found;
}

void FileIndex::set_root(const fs::path &root)
{
    std::lock_guard lock(mu);
    if (root_path == root)
        return;
    root_path = root;
    entries.clear();
    ver.fetch_add(1, std::memory_order_relaxed);
}

void FileIndex::rebuild()
{
    fs::path root;
    {
        std::lock_guard lock(mu);
        root = root_path;
    }

    auto found = scan_files(root);

    {
        std::lock_guard lock(mu);
        if (root_path != root)
            return;
        entries = std::move(found);
    }
    indexing.store(false, std::memory_order_release);
    ver.fetch_add(1, std::memory_order_relaxed);
    Logger::info(std::format("FileIndex: {} files under {}", file_count(), root.string()));
}

void FileIndex::rebuild_async()
{
    fs::path root;
    {
        std::lock_guard lock(mu);
        root = root_path;
    }
    if (root.empty())
        return;

    const std::uint64_t token = Background::instance().next_token();
    job_token.store(token, std::memory_order_relaxed);
    indexing.store(true, std::memory_order_release);

    Background::instance().post([this, root, token]() {
        if (job_token.load(std::memory_order_relaxed) != token)
            return;

        auto found = scan_files(root);

        if (job_token.load(std::memory_order_relaxed) != token)
            return;

        {
            std::lock_guard lock(mu);
            if (root_path != root)
                return;
            entries = std::move(found);
        }
        indexing.store(false, std::memory_order_release);
        ver.fetch_add(1, std::memory_order_relaxed);
        Logger::info(std::format(
            "FileIndex(async): {} files under {}",
            file_count(),
            root.string()));
    });
}

bool FileIndex::is_indexing() const
{
    return indexing.load(std::memory_order_acquire);
}

std::uint64_t FileIndex::version() const
{
    return ver.load(std::memory_order_acquire);
}

fs::path FileIndex::root() const
{
    std::lock_guard lock(mu);
    return root_path;
}

std::vector<fs::path> FileIndex::files() const
{
    std::lock_guard lock(mu);
    return entries;
}

std::size_t FileIndex::file_count() const
{
    std::lock_guard lock(mu);
    return entries.size();
}
