#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

class Messages
{
public:
    static void info(const std::string &text);
    static void warning(const std::string &text);
    static void error(const std::string &text);

    // Replace history with raw lines (no [LEVEL] prefix). Used by :help.
    static void set_lines(std::vector<std::string> lines);

    static std::vector<std::string> all();
    static void clear();

    // Ephemeral statusbar echo (not a log dump). Sequence bumps on each push.
    static std::pair<std::uint64_t, std::string> echo_snapshot();

private:
    static void push(const std::string &level, const std::string &text);
    static std::vector<std::string> entries;
    static std::mutex mu;
    static std::string last_echo;
    static std::uint64_t echo_seq;
    static constexpr std::size_t kMaxEntries = 2000;
};
