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
