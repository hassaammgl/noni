#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
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

    // Append newly read stdout bytes (thread-safe).
    std::string take_stdout();

private:
    void reader_loop();

    int stdin_fd_ = -1;
    int stdout_fd_ = -1;
    pid_t pid_ = -1;
    std::thread reader_;
    std::atomic<bool> running_{false};

    std::mutex out_mu_;
    std::string out_buf_;
};
