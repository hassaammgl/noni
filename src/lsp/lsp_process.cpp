#include <lsp/lsp_process.hpp>
#include <utils/logger.hpp>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

LspProcess::~LspProcess()
{
    stop();
}

bool LspProcess::start(const std::vector<std::string> &argv, const std::string &cwd)
{
    stop();
    if (argv.empty())
        return false;

    int in_pipe[2]{-1, -1};
    int out_pipe[2]{-1, -1};
    if (pipe(in_pipe) != 0)
    {
        Logger::error("LSP: pipe() failed");
        close(in_pipe[0]);
        close(in_pipe[1]);
        return false;
    }
    if (pipe(out_pipe) != 0)
    {
        Logger::error("LSP: pipe() failed");
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        return false;
    }

    const pid_t pid = fork();
    if (pid < 0)
    {
        Logger::error("LSP: fork() failed");
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        return false;
    }

    if (pid == 0)
    {
        // Child: stdin <- in_pipe[0], stdout -> out_pipe[1]
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        // NEVER leave stderr on the TTY — clangd (and friends) spam I[...] lines
        // that corrupt ncurses. Park them in a file (or /dev/null).
        (void)mkdir("logs", 0755);
        int err_fd = open("logs/lsp.stderr.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (err_fd < 0)
            err_fd = open("/dev/null", O_WRONLY);
        if (err_fd >= 0)
        {
            dup2(err_fd, STDERR_FILENO);
            if (err_fd != STDERR_FILENO)
                close(err_fd);
        }

        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);

        if (!cwd.empty())
            (void)chdir(cwd.c_str());

        std::vector<char *> args;
        args.reserve(argv.size() + 1);
        for (const auto &a : argv)
            args.push_back(const_cast<char *>(a.c_str()));
        args.push_back(nullptr);
        execvp(args[0], args.data());
        _exit(127);
    }

    // Parent
    close(in_pipe[0]);
    close(out_pipe[1]);
    stdin_fd_ = in_pipe[1];
    stdout_fd_ = out_pipe[0];
    pid_ = pid;
    running_ = true;
    reader_ = std::thread([this]() { reader_loop(); });
    Logger::info(std::format("LSP process started pid={} cmd={}", static_cast<int>(pid_), argv[0]));
    return true;
}

void LspProcess::stop()
{
    running_ = false;

    // Terminate the child first: its exit closes the pipe write end, which is
    // what unblocks the reader thread (close() of the read end from another
    // thread does not wake a blocked read() on Linux). Bound the grace period
    // so a server that ignores SIGTERM cannot hang shutdown.
    if (pid_ > 0)
    {
        kill(pid_, SIGTERM);
        int status = 0;
        for (int i = 0; i < 20 && pid_ > 0; ++i)
        {
            const pid_t r = waitpid(pid_, &status, WNOHANG);
            if (r == pid_)
            {
                pid_ = -1;
                break;
            }
            usleep(50000);
        }
        if (pid_ > 0)
        {
            kill(pid_, SIGKILL);
            waitpid(pid_, &status, 0);
            pid_ = -1;
        }
    }

    if (stdin_fd_ >= 0)
    {
        close(stdin_fd_);
        stdin_fd_ = -1;
    }
    if (reader_.joinable())
        reader_.join();
    // Joined reader no longer touches stdout_fd_, so there is no concurrent
    // access when we reset it here.
    if (stdout_fd_ >= 0)
    {
        close(stdout_fd_);
        stdout_fd_ = -1;
    }
}

bool LspProcess::alive() const
{
    return running_.load() && pid_ > 0;
}

bool LspProcess::write_all(const char *data, std::size_t n)
{
    if (stdin_fd_ < 0 || !data)
        return false;
    std::size_t off = 0;
    while (off < n)
    {
        const ssize_t w = ::write(stdin_fd_, data + off, n - off);
        if (w < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (w == 0)
            return false;
        off += static_cast<std::size_t>(w);
    }
    return true;
}

std::string LspProcess::take_stdout()
{
    std::lock_guard lock(out_mu_);
    std::string out = std::move(out_buf_);
    out_buf_.clear();
    return out;
}

void LspProcess::reader_loop()
{
    char buf[4096];
    while (running_.load())
    {
        if (stdout_fd_ < 0)
            break;
        const ssize_t n = ::read(stdout_fd_, buf, sizeof(buf));
        if (n > 0)
        {
            std::lock_guard lock(out_mu_);
            out_buf_.append(buf, static_cast<std::size_t>(n));
            continue;
        }
        if (n == 0)
            break;
        if (errno == EINTR)
            continue;
        break;
    }
    running_ = false;
}
