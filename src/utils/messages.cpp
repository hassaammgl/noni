#include <utils/messages.hpp>

#include <format>

std::vector<std::string> Messages::entries;
std::mutex Messages::mu;

void Messages::push(const std::string &level, const std::string &text)
{
    std::lock_guard lock(mu);
    entries.push_back(std::format("[{}] {}", level, text));
    if (entries.size() > 200)
        entries.erase(entries.begin());
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
