#include <utils/async.hpp>

Background &Background::instance()
{
    static Background bg;
    return bg;
}

Background::~Background()
{
    stop();
}

void Background::start(unsigned workers)
{
    std::lock_guard lock(mu);
    if (started)
        return;

    stop_flag = false;
    started = true;
    if (workers < 1)
        workers = 1;

    for (unsigned i = 0; i < workers; ++i)
        this->workers.emplace_back([this]() { worker_loop(); });
}

void Background::stop()
{
    {
        std::lock_guard lock(mu);
        if (!started)
            return;
        stop_flag = true;
    }
    cv.notify_all();

    for (auto &t : workers)
    {
        if (t.joinable())
            t.join();
    }
    workers.clear();

    std::lock_guard lock(mu);
    while (!jobs.empty())
        jobs.pop();
    started = false;
    stop_flag = false;
}

bool Background::running() const
{
    return started;
}

void Background::post(Job job)
{
    if (!job)
        return;

    {
        std::lock_guard lock(mu);
        if (!started)
        {
            // Fall back to sync if pool not up yet.
            job();
            return;
        }
        jobs.push(std::move(job));
    }
    cv.notify_one();
}

std::uint64_t Background::next_token()
{
    return token.fetch_add(1, std::memory_order_relaxed);
}

void Background::worker_loop()
{
    while (true)
    {
        Job job;
        {
            std::unique_lock lock(mu);
            cv.wait(lock, [this]() { return stop_flag || !jobs.empty(); });
            if (stop_flag && jobs.empty())
                return;
            job = std::move(jobs.front());
            jobs.pop();
        }

        try
        {
            job();
        }
        catch (...)
        {
            // Never let a worker die from a job exception.
        }
    }
}
