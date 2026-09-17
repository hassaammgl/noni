#include <workspace/recent_files.hpp>

#include <algorithm>
#include <system_error>

void RecentFiles::touch(const fs::path &path)
{
    if (path.empty())
        return;
    std::error_code ec;
    fs::path abs = fs::weakly_canonical(path, ec);
    if (ec)
        abs = path;

    std::lock_guard lock(mu_);
    entries_.erase(std::remove(entries_.begin(), entries_.end(), abs), entries_.end());
    entries_.insert(entries_.begin(), std::move(abs));
    if (entries_.size() > limit_)
        entries_.resize(limit_);
}

void RecentFiles::clear()
{
    std::lock_guard lock(mu_);
    entries_.clear();
}

void RecentFiles::set_limit(std::size_t n)
{
    std::lock_guard lock(mu_);
    limit_ = std::max<std::size_t>(1, n);
    if (entries_.size() > limit_)
        entries_.resize(limit_);
}

std::vector<fs::path> RecentFiles::list() const
{
    std::lock_guard lock(mu_);
    return entries_;
}
