#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

enum class PtyLifecycle
{
    Idle,
    Running,
    Exited,
    Failed,
};

// Owned PTY + shell child. Main loop pumps master fd (T7.1).
class PtySession
{
public:
    PtySession() = default;
    ~PtySession();

    PtySession(const PtySession &) = delete;
    PtySession &operator=(const PtySession &) = delete;

    // shell empty → $SHELL → /bin/sh
    bool start(int rows, int cols, const std::string &cwd, const std::string &shell = {});
    void stop();
    bool alive() const;
    PtyLifecycle lifecycle() const { return lifecycle_.load(std::memory_order_acquire); }
    std::optional<int> exit_status() const;

    void write_bytes(const char *data, std::size_t n);
    void write_byte(char c);
    void resize(int rows, int cols);

    // Drain newly read bytes (main thread only).
    std::string take_output();

    [[nodiscard]] int poll_fd() const { return master_fd.load(std::memory_order_acquire); }
    void pump_readable();

private:
    void reap_child(bool block);

    std::atomic<int> master_fd{-1};
    pid_t child_pid = -1;
    std::atomic<bool> running{false};
    std::atomic<PtyLifecycle> lifecycle_{PtyLifecycle::Idle};

    std::string out_buf;
    std::optional<int> exit_status_;
};
