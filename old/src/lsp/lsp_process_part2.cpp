#include <lsp/lsp_process.hpp>
#include <utils/logger.hpp>

#include <cerrno>
#include <cstring>
#include <unistd.h>

std::string LspProcess::take_stdout()
{
    // Main-thread only after T7.1 (no reader thread) — no lock.
    std::string out = std::move(out_buf_);
    out_buf_.clear();
    return out;
}

void LspProcess::pump_readable()
{
    if (stdout_fd_ < 0 || !running_.load())
        return;

    char buf[4096];
    for (;;)
    {
        const ssize_t n = ::read(stdout_fd_, buf, sizeof(buf));
        if (n > 0)
        {
            out_buf_.append(buf, static_cast<std::size_t>(n));
            continue;
        }
        if (n == 0)
        {
            running_ = false;
            return;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        running_ = false;
        return;
    }
}
