#include "ui_impl.hpp"

#include <utils/event_wake.hpp>

#include <cerrno>
#include <cstring>
#include <csignal>
#include <poll.h>
#include <unistd.h>
#include <vector>

namespace
{
    void on_sigwinch(int)
    {
        if (EventWake *w = EventWake::ui())
            w->notify();
    }
}

int UI::next_poll_timeout_ms() const
{
    using clock = std::chrono::steady_clock;
    const auto now = clock::now();
    int ms = -1;
    auto consider = [&](clock::time_point at) {
        if (at <= now)
        {
            ms = 0;
            return;
        }
        const auto d = std::chrono::duration_cast<std::chrono::milliseconds>(at - now).count();
        const int di = static_cast<int>(d > 60000 ? 60000 : d);
        if (ms < 0 || di < ms)
            ms = di;
    };

    if (dirty_clock_armed_ && config.autosave.mode == "afterDelay")
        consider(dirty_since_ + std::chrono::milliseconds(config.autosave.delay_ms));

    if (fs_explorer_dirty_)
        consider(fs_explorer_refresh_at_);

    bool any_dirty = false;
    for (const auto &tab : core.buffers().get_tabs())
    {
        if (tab.buffer().is_dirty())
        {
            any_dirty = true;
            break;
        }
    }
    if (any_dirty)
        consider(last_recovery_tick_ + std::chrono::seconds(2));

    return ms;
}

void UI::run()
{
    Logger::info("UI main loop started (poll)");
    sync_fs_watches();

    EventWake wake;
    EventWake::set_ui(wake.ok() ? &wake : nullptr);

    struct sigaction sa{};
    sa.sa_handler = on_sigwinch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    struct sigaction old_sa{};
    const bool hooked = (::sigaction(SIGWINCH, &sa, &old_sa) == 0);

    needs_render_ = true;
    timeout(0);

    while (running)
    {
        std::vector<pollfd> pfds;
        pfds.reserve(8);
        pfds.push_back(pollfd{.fd = STDIN_FILENO, .events = POLLIN, .revents = 0});
        if (wake.ok())
            pfds.push_back(pollfd{.fd = wake.fd(), .events = POLLIN, .revents = 0});
        if (fs_watcher_.running())
            pfds.push_back(pollfd{.fd = fs_watcher_.poll_fd(), .events = POLLIN, .revents = 0});
        if (const int tfd = terminal.poll_fd(); tfd >= 0)
            pfds.push_back(pollfd{.fd = tfd, .events = POLLIN, .revents = 0});
        core.lsp().append_poll_fds(pfds);

        const int wait_ms = needs_render_ ? 0 : next_poll_timeout_ms();
        const int pr = ::poll(pfds.data(), static_cast<nfds_t>(pfds.size()), wait_ms);
        if (pr < 0 && errno != EINTR)
            Logger::warning(std::format("poll failed: {}", std::strerror(errno)));

        bool stdin_ready = false;
        for (const auto &p : pfds)
        {
            if (!(p.revents & (POLLIN | POLLHUP | POLLERR)))
                continue;
            if (p.fd == STDIN_FILENO)
                stdin_ready = true;
            else if (wake.ok() && p.fd == wake.fd())
            {
                wake.drain();
                needs_render_ = true;
            }
            else
                needs_render_ = true;
        }
        if (pr == 0)
            needs_render_ = true; // timer fired

        if (file_picker.poll())
            needs_render_ = true;
        if (search_panel.poll())
            needs_render_ = true;
        if (terminal.poll())
            needs_render_ = true;
        poll_fs_events();
        if (sidebar.poll())
        {
            needs_render_ = true;
            if (core.buffers().has_tabs())
            {
                const fs::path path = core.buffers().active().buffer().get_buffer_path();
                if (!path.empty())
                    sidebar.reveal_path(path);
            }
        }

        if (needs_render_)
        {
            render();
            needs_render_ = false;
        }

        if (stdin_ready)
        {
            handle_inputs();
            needs_render_ = true;
        }
    }

    EventWake::set_ui(nullptr);
    if (hooked)
        ::sigaction(SIGWINCH, &old_sa, nullptr);
    Logger::info("UI main loop stopped");
}
