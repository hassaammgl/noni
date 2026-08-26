#include <utils/str.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace StrUtils
{
    std::vector<std::string> split(const std::string &str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::istringstream iss(str);
        std::string token;
        while (std::getline(iss, token, delimiter))
        {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::string trim_left(std::string_view str)
    {
        auto start = str.begin();

        while (
            start != str.end() &&
            std::isspace(static_cast<unsigned char>(*start)))
        {
            ++start;
        }

        return std::string(start, str.end());
    }

    std::string trim_right(std::string_view str)
    {
        auto end = str.end();

        while (end != str.begin())
        {
            auto previous = end - 1;

            if (
                !std::isspace(
                    static_cast<unsigned char>(*previous)))
            {
                break;
            }

            end = previous;
        }

        return std::string(str.begin(), end);
    }

    std::string trim(std::string_view str)
    {
        return trim_right(trim_left(str));
    }

    std::string to_lower(std::string_view str)
    {
        std::string result(str);

        std::ranges::transform(
            result,
            result.begin(),
            [](unsigned char character)
            {
                return std::tolower(character);
            });

        return result;
    }

    std::string to_upper(std::string_view str)
    {
        std::string result(str);

        std::ranges::transform(
            result,
            result.begin(),
            [](unsigned char character)
            {
                return std::toupper(character);
            });

        return result;
    }

    bool starts_with(std::string_view str, std::string_view prefix)
    {
        return str.starts_with(prefix);
    }

    bool ends_with(std::string_view str, std::string_view suffix)
    {
        return str.ends_with(suffix);
    }

    bool contains(std::string_view str, std::string_view value)
    {
        return str.find(value) != std::string_view::npos;
    }

    std::string join(const std::vector<std::string> &items, std::string_view delimiter)
    {
        if (items.empty())
        {
            return "";
        }

        std::string result;

        for (std::size_t i = 0; i < items.size(); ++i)
        {
            result += items[i];

            if (i < items.size() - 1)
            {
                result += delimiter;
            }
        }

        return result;
    }

    std::string replace(std::string str, std::string_view from, std::string_view to)
    {
        if (from.empty())
        {
            return str;
        }

        std::size_t position = 0;

        while (
            (position = str.find(from, position)) != std::string::npos)
        {
            str.replace(
                position,
                from.length(),
                to);

            position += to.length();
        }

        return str;
    }

    bool is_empty(std::string_view str)
    {
        return str.empty();
    }

    bool is_whitespace(std::string_view str)
    {
        return std::ranges::all_of(
            str,
            [](unsigned char character)
            {
                return std::isspace(character);
            });
    }

    std::size_t count(std::string_view str, char character)
    {
        return std::ranges::count(
            str,
            character);
    }

    std::string pad_left(std::string_view str, std::size_t length, char fill)
    {
        if (str.size() >= length)
        {
            return std::string(str);
        }

        return std::string(length - str.size(), fill) + std::string(str);
    }

    std::string pad_right(std::string_view str, std::size_t length, char fill)
    {
        std::string result(str);

        if (result.size() < length)
        {
            result.append(
                length - result.size(),
                fill);
        }

        return result;
    }

    std::string remove_whitespace(std::string_view str)
    {
        std::string result;

        for (unsigned char character : str)
        {
            if (!std::isspace(character))
            {
                result += character;
            }
        }

        return result;
    }

    std::string repeat(std::string_view str, std::size_t times)
    {
        std::string result;

        result.reserve(
            str.size() * times);

        for (std::size_t i = 0; i < times; ++i)
        {
            result += str;
        }

        return result;
    }
}