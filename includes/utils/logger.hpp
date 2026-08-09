#pragma once
#include <string>

class Logger
{
private:
    void log(const std::string &level, const std::string &message);

public:
    void debug(const std::string &message);
    void info(const std::string &message);
    void error(const std::string &message);
    void warning(const std::string &message);
};