#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

enum class PtyLifecycle
{
    Idle,
    Running,
    Exited,
    Failed,
};

// Owned PTY + shell child. Reader thread feeds output into a buffer.
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

    // Drain newly read bytes (thread-safe).
    std::string take_output();

private:
    void reader_loop();
    void reap_child(bool block);

    std::atomic<int> master_fd{-1};
    pid_t child_pid = -1;
    std::thread reader;
    std::atomic<bool> running{false};
    std::atomic<PtyLifecycle> lifecycle_{PtyLifecycle::Idle};

    std::mutex out_mu;
    std::string out_buf;
    std::optional<int> exit_status_;
};
