#include <utils/fs_watcher.hpp>
#include <utils/logger.hpp>

#include <cerrno>
#include <cstring>
#include <format>
#include <sys/inotify.h>
#include <unistd.h>

namespace
{
    uint32_t mask_for(const fs::path &path, bool is_dir)
    {
        (void)path;
        if (is_dir)
        {
            return IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO |
                   IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;
        }
        return IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB | IN_DELETE_SELF |
               IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO;
    }

    FsEventKind kind_from(uint32_t mask)
    {
        if (mask & (IN_DELETE | IN_DELETE_SELF))
            return FsEventKind::Deleted;
        if (mask & (IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF))
            return FsEventKind::Moved;
        if (mask & IN_CREATE)
            return FsEventKind::Created;
        if (mask & (IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB))
            return FsEventKind::Modified;
        return FsEventKind::Other;
    }
}

FsWatcher::~FsWatcher()
{
    stop();
}

bool FsWatcher::start()
{
    if (fd_ >= 0)
        return true;

    fd_ = ::inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (fd_ < 0)
    {
        Logger::error(std::format("FsWatcher: inotify_init1 failed: {}", std::strerror(errno)));
        return false;
    }
    Logger::info("FsWatcher started");
    return true;
}

void FsWatcher::stop()
{
    std::lock_guard lock(mu_);
    if (fd_ < 0)
        return;

    for (const auto &[wd, path] : wd_to_path_)
        (void)::inotify_rm_watch(fd_, wd);
    wd_to_path_.clear();
    path_to_wd_.clear();
    workspace_.clear();

    ::close(fd_);
    fd_ = -1;
    Logger::info("FsWatcher stopped");
}

bool FsWatcher::should_ignore(const fs::path &path)
{
    const std::string name = path.filename().string();
    if (name == ".git" || name == "logs" || name == "build" || name == "node_modules" ||
        name == ".cache" || name == "__pycache__")
        return true;
    if (name.find(".noni.") != std::string::npos)
        return true;
    if (name.size() > 1 && name[0] == '.' && name != ".")
    {
        // Ignore common editor swap / backup noise under watch roots.
        if (name.ends_with("~") || name.ends_with(".swp") || name.ends_with(".swo"))
            return true;
    }
    return false;
}

bool FsWatcher::add_watch_unlocked(const fs::path &path, uint32_t mask)
{
    if (fd_ < 0 || path.empty())
        return false;

    std::error_code ec;
    fs::path canon = fs::weakly_canonical(path, ec);
    if (ec)
        canon = path;
    const std::string key = canon.string();
    if (path_to_wd_.contains(key))
        return true;
    if (should_ignore(canon))
        return false;

    const int wd = ::inotify_add_watch(fd_, key.c_str(), mask);
    if (wd < 0)
    {
        Logger::debug(std::format(
            "FsWatcher: add_watch failed for {}: {}",
            key,
            std::strerror(errno)));
        return false;
    }

    wd_to_path_[wd] = canon;
    path_to_wd_[key] = wd;
    return true;
}

void FsWatcher::remove_wd_unlocked(int wd)
{
    auto it = wd_to_path_.find(wd);
    if (it == wd_to_path_.end())
        return;
    const std::string key = it->second.string();
    wd_to_path_.erase(it);
    path_to_wd_.erase(key);
    if (fd_ >= 0)
        (void)::inotify_rm_watch(fd_, wd);
}

bool FsWatcher::watch(const fs::path &path)
{
    if (!start())
        return false;

    std::error_code ec;
    const bool is_dir = fs::is_directory(path, ec);
    std::lock_guard lock(mu_);
    return add_watch_unlocked(path, mask_for(path, is_dir && !ec));
}

void FsWatcher::unwatch(const fs::path &path)
{
    std::lock_guard lock(mu_);
    std::error_code ec;
    fs::path canon = fs::weakly_canonical(path, ec);
    if (ec)
        canon = path;
    const auto it = path_to_wd_.find(canon.string());
    if (it == path_to_wd_.end())
        return;
    remove_wd_unlocked(it->second);
}

void FsWatcher::clear()
{
    std::lock_guard lock(mu_);
    if (fd_ < 0)
        return;
    for (const auto &[wd, path] : wd_to_path_)
        (void)::inotify_rm_watch(fd_, wd);
    wd_to_path_.clear();
    path_to_wd_.clear();
    workspace_.clear();
}

void FsWatcher::set_workspace(const fs::path &root)
{
    if (!start())
        return;

    std::lock_guard lock(mu_);
    if (!workspace_.empty())
    {
        const auto it = path_to_wd_.find(workspace_.string());
        if (it != path_to_wd_.end())
            remove_wd_unlocked(it->second);
        workspace_.clear();
    }

    if (root.empty())
        return;

    std::error_code ec;
    fs::path canon = fs::weakly_canonical(root, ec);
    if (ec)
        canon = root;
    if (!fs::is_directory(canon, ec))
        return;

    if (add_watch_unlocked(canon, mask_for(canon, true)))
        workspace_ = canon;
}

std::vector<FsEvent> FsWatcher::poll()
{
    std::vector<FsEvent> out;
    if (fd_ < 0)
        return out;

    alignas(struct inotify_event) char buf[8192];
    while (true)
    {
        const ssize_t n = ::read(fd_, buf, sizeof(buf));
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            Logger::warning(std::format("FsWatcher: read failed: {}", std::strerror(errno)));
            break;
        }
        if (n == 0)
            break;

        std::lock_guard lock(mu_);
        for (ssize_t off = 0; off < n;)
        {
            const auto *ev = reinterpret_cast<const struct inotify_event *>(buf + off);
            off += static_cast<ssize_t>(sizeof(struct inotify_event) + ev->len);

            if (ev->mask & IN_Q_OVERFLOW)
            {
                Logger::warning("FsWatcher: event queue overflow");
                continue;
            }

            auto it = wd_to_path_.find(ev->wd);
            if (it == wd_to_path_.end())
                continue;

            fs::path full = it->second;
            if (ev->len > 0 && ev->name[0] != '\0')
                full /= ev->name;

            if (should_ignore(full) || should_ignore(full.filename()))
                continue;

            FsEvent fe;
            fe.kind = kind_from(ev->mask);
            fe.path = std::move(full);
            fe.is_dir = (ev->mask & IN_ISDIR) != 0;
            out.push_back(std::move(fe));

            if (ev->mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_IGNORED))
                remove_wd_unlocked(ev->wd);
        }
    }
    return out;
}
