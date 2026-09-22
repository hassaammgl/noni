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
#include <poll.h>

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
    std::lock_guard lock(out_mu);
    std::string out;
    out.swap(out_buf);
    return out;
}

void PtySession::reader_loop()
{
    char buf[4096];
    while (running.load(std::memory_order_acquire))
    {
        if (master_fd.load() < 0)
            break;

        pollfd pfd{};
        pfd.fd = master_fd.load();
        pfd.events = POLLIN;
        const int pr = poll(&pfd, 1, 50);
        if (pr < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        if (pr == 0)
            continue;

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
            break;

        const ssize_t n = ::read(master_fd.load(), buf, sizeof(buf));
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                continue;
            break;
        }
        if (n == 0)
            break;

        {
            std::lock_guard lock(out_mu);
            out_buf.append(buf, static_cast<std::size_t>(n));
            constexpr std::size_t kMax = 512 * 1024;
            if (out_buf.size() > kMax)
                out_buf.erase(0, out_buf.size() - kMax);
        }
    }

    running.store(false, std::memory_order_release);
    if (lifecycle_.load(std::memory_order_acquire) == PtyLifecycle::Running)
        lifecycle_.store(PtyLifecycle::Exited, std::memory_order_release);

    // Sole owner: this thread is the only user of master_fd (O_NONBLOCK means
    // read() never blocks; poll() times out in ≤50ms), so it also closes it.
    // No other thread can close or reuse the fd while it is in use here.
    const int fd = master_fd.exchange(-1);
    if (fd >= 0)
        ::close(fd);
}
