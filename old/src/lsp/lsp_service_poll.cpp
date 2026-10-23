#include "lsp_internal.hpp"

#include <poll.h>

void LspService::append_poll_fds(std::vector<pollfd> &out) const
{
    std::lock_guard lock(mu_);
    for (const auto &[k, s] : sessions_)
    {
        if (!s)
            continue;
        const int fd = s->poll_fd();
        if (fd < 0)
            continue;
        pollfd p{};
        p.fd = fd;
        p.events = POLLIN;
        out.push_back(p);
    }
}
