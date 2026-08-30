#pragma once

#include <source_location>
#include <string>

class Logger
{
public:
    static void init();

    static void info(
        const std::string &message,
        const std::source_location &loc = std::source_location::current());

    static void error(
        const std::string &message,
        const std::source_location &loc = std::source_location::current());

    static void warning(
        const std::string &message,
        const std::source_location &loc = std::source_location::current());

    static void debug(
        const std::string &message,
        const std::source_location &loc = std::source_location::current());

private:
    static void log(
        const std::string &level,
        const std::string &message,
        const std::source_location &loc);

    static void write_line(const std::string &line);
    static void ensure_open();
};
