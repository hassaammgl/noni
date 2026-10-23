#include <utils/fs_watcher.hpp>
#include <utils/logger.hpp>

#include <cerrno>
#include <cstring>
#include <format>
#include <sys/inotify.h>
#include <unistd.h>

#include "fs_watcher_detail.hpp"

using namespace fs_watcher_detail;

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
