#include <utils/logger.hpp>

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace
{
constexpr const char *LOG_DIR = "logs";
constexpr const char *LOG_PATH = "logs/noni.log";

std::ofstream log_file;
bool log_ready = false;

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

void Logger::init()
{
    fs::create_directories(LOG_DIR);

    if (log_file.is_open())
        log_file.close();

    log_file.open(LOG_PATH, std::ios::out | std::ios::trunc);
    log_ready = log_file.is_open();

    write_line(std::format("[{}] [INFO] --- noni session started ---", timestamp()));
}

void Logger::ensure_open()
{
    if (log_ready)
        return;

    fs::create_directories(LOG_DIR);
    log_file.open(LOG_PATH, std::ios::app);
    log_ready = log_file.is_open();
}

void Logger::write_line(const std::string &line)
{
    ensure_open();

    if (!log_ready)
        return;

    log_file << line << '\n';
    log_file.flush();
}

void Logger::log(
    const std::string &level,
    const std::string &message,
    const std::source_location &loc)
{
    const std::string ts = timestamp();

    write_line(std::format("[{}] [{}] {}", ts, level, message));
    write_line(std::format(
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
