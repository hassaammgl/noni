#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

// Owned PTY + shell child. Reader thread feeds output into a buffer.
class PtySession
{
public:
    PtySession() = default;
    ~PtySession();

    PtySession(const PtySession &) = delete;
    PtySession &operator=(const PtySession &) = delete;

    bool start(int rows, int cols, const std::string &cwd);
    void stop();
    bool alive() const;

    void write_bytes(const char *data, std::size_t n);
    void write_byte(char c);
    void resize(int rows, int cols);

    // Drain newly read bytes (thread-safe).
    std::string take_output();

private:
    void reader_loop();

    int master_fd = -1;
    pid_t child_pid = -1;
    std::thread reader;
    std::atomic<bool> running{false};

    std::mutex out_mu;
    std::string out_buf;
};
