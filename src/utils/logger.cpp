#include <utils/logger.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace
{
constexpr const char *LOG_DIR = "logs";
constexpr const char *LOG_PATH = "logs/noni.log";

std::ofstream log_file;
bool log_ready = false;

std::mutex queue_mu;
std::condition_variable queue_cv;
std::queue<std::string> queue;
std::thread writer;
std::atomic<bool> running{false};

std::string timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};

#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return out.str();
}
} // namespace

void Logger::ensure_open()
{
    if (log_ready)
        return;

    fs::create_directories(LOG_DIR);
    log_file.open(LOG_PATH, std::ios::app);
    log_ready = log_file.is_open();
}

void Logger::writer_loop()
{
    std::vector<std::string> batch;
    batch.reserve(64);

    while (true)
    {
        {
            std::unique_lock lock(queue_mu);
            queue_cv.wait_for(lock, std::chrono::milliseconds(100), []() {
                return !queue.empty() || !running.load(std::memory_order_acquire);
            });

            while (!queue.empty())
            {
                batch.push_back(std::move(queue.front()));
                queue.pop();
            }

            if (batch.empty() && !running.load(std::memory_order_acquire))
                return;
        }

        if (batch.empty())
            continue;

        ensure_open();
        if (log_ready)
        {
            for (const auto &line : batch)
                log_file << line << '\n';
            log_file.flush();
        }
        batch.clear();
    }
}

void Logger::enqueue(std::string line)
{
    if (!running.load(std::memory_order_acquire))
    {
        // Before init / after shutdown: best-effort sync write.
        ensure_open();
        if (log_ready)
        {
            log_file << line << '\n';
            log_file.flush();
        }
        return;
    }

    {
        std::lock_guard lock(queue_mu);
        queue.push(std::move(line));
    }
    queue_cv.notify_one();
}

void Logger::init()
{
    fs::create_directories(LOG_DIR);

    if (log_file.is_open())
        log_file.close();

    log_file.open(LOG_PATH, std::ios::out | std::ios::trunc);
    log_ready = log_file.is_open();

    if (running.exchange(true))
        return;

    writer = std::thread(writer_loop);
    enqueue(std::format("[{}] [INFO] --- noni session started ---", timestamp()));
}

void Logger::shutdown()
{
    if (!running.exchange(false))
        return;

    queue_cv.notify_all();
    if (writer.joinable())
        writer.join();

    if (log_file.is_open())
    {
        log_file.flush();
        log_file.close();
    }
    log_ready = false;
}

void Logger::log(
    const std::string &level,
    const std::string &message,
    const std::source_location &loc)
{
    const std::string ts = timestamp();

    enqueue(std::format("[{}] [{}] {}", ts, level, message));
    enqueue(std::format(
        "[{}] [{}] {}:{} in {}",
        ts,
        level,
        loc.file_name(),
        loc.line(),
        loc.function_name()));
}

void Logger::info(
    const std::string &message,
    const std::source_location &loc)
{
    log("INFO", message, loc);
}

void Logger::error(
    const std::string &message,
    const std::source_location &loc)
{
    log("ERROR", message, loc);
}

void Logger::warning(
    const std::string &message,
    const std::source_location &loc)
{
    log("WARNING", message, loc);
}

void Logger::debug(
    const std::string &message,
    const std::source_location &loc)
{
    log("DEBUG", message, loc);
}
