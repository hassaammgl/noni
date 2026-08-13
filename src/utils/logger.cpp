#include "utils/logger.hpp"

#include <format>
#include <iostream>

void Logger::log(
    const std::string &level,
    const std::string &message,
    const std::source_location &loc)
{
    std::cout << std::format(
        "[{}:{}] [{}] [{}] {}\n",
        level,
        loc.file_name(),
        loc.line(),
        loc.function_name(),
        message);
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