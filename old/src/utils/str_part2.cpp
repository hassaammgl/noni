#include <utils/str.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>


namespace StrUtils
{
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
