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

PtySession::~PtySession()
{
    stop();
}

bool PtySession::start(int rows, int cols, const std::string &cwd)
{
    stop();

    if (rows < 1)
        rows = 1;
    if (cols < 1)
        cols = 1;

    winsize ws{};
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_col = static_cast<unsigned short>(cols);

    int master = -1;
    const pid_t pid = forkpty(&master, nullptr, nullptr, &ws);
    if (pid < 0)
    {
        Logger::error(std::format("forkpty failed: {}", std::strerror(errno)));
        return false;
    }

    if (pid == 0)
    {
        // Child: interactive shell
        if (!cwd.empty())
            (void)::chdir(cwd.c_str());

        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);

        const char *shell = std::getenv("SHELL");
        if (!shell || !*shell)
            shell = "/bin/bash";

        execl(shell, shell, "-l", static_cast<char *>(nullptr));
        execl(shell, shell, static_cast<char *>(nullptr));
        execl("/bin/sh", "sh", static_cast<char *>(nullptr));
        _exit(127);
    }

    // Parent
    master_fd = master;
    child_pid = pid;

    const int flags = fcntl(master_fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

    running.store(true, std::memory_order_release);
    reader = std::thread([this]() { reader_loop(); });

    Logger::info(std::format("PTY started pid={} {}x{}", pid, cols, rows));
    return true;
}

void PtySession::stop()
{
    const bool was = running.exchange(false, std::memory_order_acq_rel);
    if (!was && master_fd < 0 && child_pid < 0)
        return;

    if (master_fd >= 0)
    {
        ::close(master_fd);
        master_fd = -1;
    }

    if (reader.joinable())
        reader.join();

    if (child_pid > 0)
    {
        kill(child_pid, SIGHUP);
        int status = 0;
        waitpid(child_pid, &status, WNOHANG);
        // Best-effort reap
        for (int i = 0; i < 20; ++i)
        {
            const pid_t r = waitpid(child_pid, &status, WNOHANG);
            if (r == child_pid || (r < 0 && errno == ECHILD))
                break;
            usleep(10000);
        }
        if (waitpid(child_pid, &status, WNOHANG) == 0)
        {
            kill(child_pid, SIGKILL);
            waitpid(child_pid, &status, 0);
        }
        child_pid = -1;
    }

    std::lock_guard lock(out_mu);
    out_buf.clear();
}

bool PtySession::alive() const
{
    return running.load(std::memory_order_acquire) && child_pid > 0;
}

void PtySession::write_bytes(const char *data, std::size_t n)
{
    if (master_fd < 0 || !data || n == 0)
        return;

    std::size_t off = 0;
    while (off < n)
    {
        const ssize_t w = ::write(master_fd, data + off, n - off);
        if (w < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                usleep(1000);
                continue;
            }
            break;
        }
        if (w == 0)
            break;
        off += static_cast<std::size_t>(w);
    }
}

void PtySession::write_byte(char c)
{
    write_bytes(&c, 1);
}

void PtySession::resize(int rows, int cols)
{
    if (master_fd < 0)
        return;
    if (rows < 1)
        rows = 1;
    if (cols < 1)
        cols = 1;

    winsize ws{};
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_col = static_cast<unsigned short>(cols);
    ioctl(master_fd, TIOCSWINSZ, &ws);
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
        if (master_fd < 0)
            break;

        pollfd pfd{};
        pfd.fd = master_fd;
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

        const ssize_t n = ::read(master_fd, buf, sizeof(buf));
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
            // Cap memory if consumer is slow.
            constexpr std::size_t kMax = 512 * 1024;
            if (out_buf.size() > kMax)
                out_buf.erase(0, out_buf.size() - kMax);
        }
    }

    running.store(false, std::memory_order_release);
}
