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

bool PtySession::start(int rows, int cols, const std::string &cwd, const std::string &shell)
{
    stop();
    exit_status_.reset();
    lifecycle_.store(PtyLifecycle::Idle, std::memory_order_release);

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
        lifecycle_.store(PtyLifecycle::Failed, std::memory_order_release);
        return false;
    }

    if (pid == 0)
    {
        if (!cwd.empty())
            (void)::chdir(cwd.c_str());

        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);

        const char *exec_shell = nullptr;
        if (!shell.empty())
            exec_shell = shell.c_str();
        if (!exec_shell || !*exec_shell)
            exec_shell = std::getenv("SHELL");
        if (!exec_shell || !*exec_shell)
            exec_shell = "/bin/sh";

        execl(exec_shell, exec_shell, "-l", static_cast<char *>(nullptr));
        execl(exec_shell, exec_shell, static_cast<char *>(nullptr));
        execl("/bin/sh", "sh", static_cast<char *>(nullptr));
        _exit(127);
    }

    master_fd.store(master);
    child_pid = pid;

    const int flags = fcntl(master_fd.load(), F_GETFL, 0);
    if (flags >= 0)
        fcntl(master_fd.load(), F_SETFL, flags | O_NONBLOCK);

    running.store(true, std::memory_order_release);
    lifecycle_.store(PtyLifecycle::Running, std::memory_order_release);
    reader = std::thread([this]() { reader_loop(); });

    Logger::info(std::format("PTY started pid={} shell={} {}x{}", pid, shell.empty() ? "$SHELL|/bin/sh" : shell, cols, rows));
    return true;
}

void PtySession::reap_child(bool block)
{
    if (child_pid <= 0)
        return;

    int status = 0;
    const int flags = block ? 0 : WNOHANG;
    const pid_t r = waitpid(child_pid, &status, flags);
    if (r == child_pid)
    {
        if (WIFEXITED(status))
            exit_status_ = WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            exit_status_ = 128 + WTERMSIG(status);
        else
            exit_status_ = -1;
        child_pid = -1;
        lifecycle_.store(PtyLifecycle::Exited, std::memory_order_release);
    }
}

void PtySession::stop()
{
    const bool was = running.exchange(false, std::memory_order_acq_rel);
    if (!was && master_fd.load() < 0 && child_pid < 0)
        return;

    if (reader.joinable())
        reader.join();

    // The reader thread closes master_fd itself on exit; exchange keeps close
    // ownership unique. This fallback only fires when no reader thread ever
    // started (e.g. std::thread creation threw after a successful forkpty).
    const int fd = master_fd.exchange(-1);
    if (fd >= 0)
        ::close(fd);

    if (child_pid > 0)
    {
        kill(child_pid, SIGHUP);
        for (int i = 0; i < 20; ++i)
        {
            reap_child(false);
            if (child_pid <= 0)
                break;
            usleep(10000);
        }
        if (child_pid > 0)
        {
            kill(child_pid, SIGKILL);
            reap_child(true);
        }
    }

    std::lock_guard lock(out_mu);
    out_buf.clear();
}

bool PtySession::alive() const
{
    return lifecycle_.load(std::memory_order_acquire) == PtyLifecycle::Running && child_pid > 0;
}

std::optional<int> PtySession::exit_status() const
{
    return exit_status_;
}

void PtySession::write_bytes(const char *data, std::size_t n)
{
    if (master_fd.load() < 0 || !data || n == 0)
        return;

    std::size_t off = 0;
    while (off < n)
    {
        const ssize_t w = ::write(master_fd.load(), data + off, n - off);
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
