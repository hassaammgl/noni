#include <utils/messages.hpp>

#include <format>
#include <utility>

std::vector<std::string> Messages::entries;
std::mutex Messages::mu;

void Messages::push(const std::string &level, const std::string &text)
{
    std::lock_guard lock(mu);
    entries.push_back(std::format("[{}] {}", level, text));
    if (entries.size() > kMaxEntries)
        entries.erase(entries.begin(), entries.begin() + static_cast<std::ptrdiff_t>(entries.size() - kMaxEntries));
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
}
