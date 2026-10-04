#pragma once

// Non-blocking wake for the UI poll() loop (Background job done, SIGWINCH).
// Linux: eventfd. Not for Logger.
class EventWake
{
public:
    EventWake();
    ~EventWake();

    EventWake(const EventWake &) = delete;
    EventWake &operator=(const EventWake &) = delete;

    [[nodiscard]] int fd() const { return fd_; }
    [[nodiscard]] bool ok() const { return fd_ >= 0; }

    void notify() noexcept;
    void drain() noexcept;

    // Process-wide UI wake pointer (set for the duration of UI::run).
    static void set_ui(EventWake *w) noexcept;
    static EventWake *ui() noexcept;

private:
    int fd_ = -1;
    int write_fd_ = -1; // pipe write end; -1 when using eventfd
};
