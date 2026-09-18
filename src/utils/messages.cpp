#include <utils/messages.hpp>

#include <format>
#include <utility>

std::vector<std::string> Messages::entries;
std::mutex Messages::mu;
std::string Messages::last_echo;
std::uint64_t Messages::echo_seq = 0;

void Messages::push(const std::string &level, const std::string &text)
{
    std::lock_guard lock(mu);
    const std::string line = std::format("[{}] {}", level, text);
    entries.push_back(line);
    if (entries.size() > kMaxEntries)
        entries.erase(entries.begin(), entries.begin() + static_cast<std::ptrdiff_t>(entries.size() - kMaxEntries));
    last_echo = line;
    ++echo_seq;
}

void Messages::info(const std::string &text)
{
    push("INFO", text);
}

void Messages::warning(const std::string &text)
{
    push("WARNING", text);
}

void Messages::error(const std::string &text)
{
    push("ERROR", text);
}

void Messages::set_lines(std::vector<std::string> lines)
{
    std::lock_guard lock(mu);
    if (lines.size() > kMaxEntries)
        lines.erase(lines.begin(), lines.begin() + static_cast<std::ptrdiff_t>(lines.size() - kMaxEntries));
    entries = std::move(lines);
    // Help dumps are not statusbar echoes.
}

std::vector<std::string> Messages::all()
{
    std::lock_guard lock(mu);
    return entries;
}

void Messages::clear()
{
    std::lock_guard lock(mu);
    entries.clear();
    last_echo.clear();
    ++echo_seq;
}

std::pair<std::uint64_t, std::string> Messages::echo_snapshot()
{
    std::lock_guard lock(mu);
    return {echo_seq, last_echo};
}
