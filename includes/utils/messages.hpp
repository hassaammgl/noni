#pragma once
#include <mutex>
#include <string>
#include <vector>

class Messages
{
public:
    static void info(const std::string &text);
    static void warning(const std::string &text);
    static void error(const std::string &text);

    static std::vector<std::string> all();
    static void clear();

private:
    static void push(const std::string &level, const std::string &text);
    static std::vector<std::string> entries;
    static std::mutex mu;
};