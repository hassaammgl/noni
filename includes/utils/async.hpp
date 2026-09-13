#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// Small background job pool for non-UI work (indexing, git, scans).
class Background
{
public:
    using Job = std::function<void()>;

    static Background &instance();

    void start(unsigned workers = 2);
    void stop();
    bool running() const;

    void post(Job job);

    // Monotonic token so callers can ignore stale results.
    std::uint64_t next_token();

private:
    Background() = default;
    ~Background();

    Background(const Background &) = delete;
    Background &operator=(const Background &) = delete;

    void worker_loop();

    std::mutex mu;
    std::condition_variable cv;
    std::queue<Job> jobs;
    std::vector<std::thread> workers;
    bool stop_flag = false;
    bool started = false;
    std::atomic<std::uint64_t> token{1};
};
