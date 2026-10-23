#include <utils/event_wake.hpp>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/eventfd.h>
#endif

namespace
{
    std::atomic<EventWake *> g_ui_wake{nullptr};

    void set_cloexec_nonblock(int fd)
    {
        if (fd < 0)
            return;
        const int fl = ::fcntl(fd, F_GETFL, 0);
        if (fl >= 0)
            ::fcntl(fd, F_SETFL, fl | O_NONBLOCK);
        const int fdfl = ::fcntl(fd, F_GETFD, 0);
        if (fdfl >= 0)
            ::fcntl(fd, F_SETFD, fdfl | FD_CLOEXEC);
    }
}

EventWake::EventWake()
{
#if defined(__linux__)
    fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd_ >= 0)
        return;
#endif
    int p[2]{-1, -1};
    if (::pipe(p) != 0)
        return;
    set_cloexec_nonblock(p[0]);
    set_cloexec_nonblock(p[1]);
    fd_ = p[0];
    write_fd_ = p[1];
}

EventWake::~EventWake()
{
    if (g_ui_wake.load(std::memory_order_acquire) == this)
        g_ui_wake.store(nullptr, std::memory_order_release);
    if (fd_ >= 0)
        ::close(fd_);
    if (write_fd_ >= 0 && write_fd_ != fd_)
        ::close(write_fd_);
    fd_ = -1;
    write_fd_ = -1;
}

void EventWake::set_ui(EventWake *w) noexcept
{
    g_ui_wake.store(w, std::memory_order_release);
}

EventWake *EventWake::ui() noexcept
{
    return g_ui_wake.load(std::memory_order_acquire);
}

void EventWake::notify() noexcept
{
    const int w = write_fd_ >= 0 ? write_fd_ : fd_;
    if (w < 0)
        return;
#if defined(__linux__)
    if (write_fd_ < 0)
    {
        std::uint64_t one = 1;
        while (::write(w, &one, sizeof(one)) < 0 && errno == EINTR)
        {
        }
        return;
    }
#endif
    char b = 1;
    while (::write(w, &b, 1) < 0 && errno == EINTR)
    {
    }
}

void EventWake::drain() noexcept
{
    if (fd_ < 0)
        return;
#if defined(__linux__)
    if (write_fd_ < 0)
    {
        std::uint64_t v = 0;
        while (::read(fd_, &v, sizeof(v)) > 0)
        {
        }
        return;
    }
#endif
    char buf[64];
    while (::read(fd_, buf, sizeof(buf)) > 0)
    {
    }
}
