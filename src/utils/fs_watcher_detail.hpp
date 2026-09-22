#pragma once
#include <utils/fs_watcher.hpp>
#include <utils/logger.hpp>

#include <cerrno>
#include <cstring>
#include <format>
#include <sys/inotify.h>
#include <unistd.h>

namespace fs_watcher_detail
{
    inline uint32_t mask_for(const fs::path &path, bool is_dir)
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

    inline FsEventKind kind_from(uint32_t mask)
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
