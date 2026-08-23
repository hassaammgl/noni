#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace StrUtils
{
    // Whitespace
    std::string trim(std::string_view str);
    std::string trimLeft(std::string_view str);
    std::string trimRight(std::string_view str);

    // Case conversion
    std::string toLower(std::string_view str);
    std::string toUpper(std::string_view str);

    // Searching
    bool startsWith(std::string_view str, std::string_view prefix);
    bool endsWith(std::string_view str, std::string_view suffix);
    bool contains(std::string_view str, std::string_view value);

    // Splitting / Joining
    std::vector<std::string> split(const std::string &str, char delimiter);
    std::string join(const std::vector<std::string> &items, std::string_view delimiter);

    // Replacement
    std::string replace(std::string str, std::string_view from, std::string_view to);

    // Checks
    bool isEmpty(std::string_view str);
    bool isWhitespace(std::string_view str);

    // Counting
    std::size_t count(std::string_view str, char character);

    // Padding
    std::string padLeft(std::string_view str, std::size_t length, char fill = ' ');

    std::string padRight(std::string_view str, std::size_t length, char fill = ' ');

    // Remove
    std::string removeWhitespace(std::string_view str);

    // Repeating
    std::string repeat(std::string_view str, std::size_t times);

}