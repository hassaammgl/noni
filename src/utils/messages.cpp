#include <utils/messages.hpp>

#include <format>

std::vector<std::string> Messages::entries;

void Messages::push(const std::string &level, const std::string &text)
{
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

const std::vector<std::string> &Messages::all()
{
    return entries;
}

void Messages::clear()
{
    entries.clear();
}
