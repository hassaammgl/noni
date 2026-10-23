#include <terminal/pty_session.hpp>
#include <utils/logger.hpp>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <format>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__linux__)
#include <pty.h>
#else
#include <util.h>
#endif

void PtySession::write_byte(char c)
{
    write_bytes(&c, 1);
}

void PtySession::resize(int rows, int cols)
{
    if (master_fd.load() < 0)
        return;
    if (rows < 1)
        rows = 1;
    if (cols < 1)
        cols = 1;

    winsize ws{};
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_col = static_cast<unsigned short>(cols);
    ioctl(master_fd.load(), TIOCSWINSZ, &ws);
}

std::string PtySession::take_output()
{
    std::string out;
    out.swap(out_buf);
    return out;
}

void PtySession::pump_readable()
{
    const int fd = master_fd.load(std::memory_order_acquire);
    if (fd < 0 || !running.load(std::memory_order_acquire))
        return;

    char buf[4096];
    for (;;)
    {
        const ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n > 0)
        {
            out_buf.append(buf, static_cast<std::size_t>(n));
            constexpr std::size_t kMax = 512 * 1024;
            if (out_buf.size() > kMax)
                out_buf.erase(0, out_buf.size() - kMax);
            continue;
        }
        if (n == 0)
            break;
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return;
        break;
    }

    running.store(false, std::memory_order_release);
    if (lifecycle_.load(std::memory_order_acquire) == PtyLifecycle::Running)
        lifecycle_.store(PtyLifecycle::Exited, std::memory_order_release);
    const int closed = master_fd.exchange(-1);
    if (closed >= 0)
        ::close(closed);
    reap_child(false);
}
