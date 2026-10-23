#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

// Bidirectional stdio child process for language servers (not a PTY).
class LspProcess
{
public:
    LspProcess() = default;
    ~LspProcess();

    LspProcess(const LspProcess &) = delete;
    LspProcess &operator=(const LspProcess &) = delete;

    bool start(const std::vector<std::string> &argv, const std::string &cwd);
    void stop();
    bool alive() const;

    bool write_all(const char *data, std::size_t n);
    bool write_all(const std::string &s) { return write_all(s.data(), s.size()); }

    // Drain stdout buffer (main thread only — T7.1/T7.6).
    std::string take_stdout();

    // Main-loop poll: LS stdout fd (-1 if idle).
    [[nodiscard]] int poll_fd() const { return stdout_fd_; }
    void pump_readable();

private:
    int stdin_fd_ = -1;
    int stdout_fd_ = -1;
    pid_t pid_ = -1;
    std::atomic<bool> running_{false};
    std::string out_buf_;
};
