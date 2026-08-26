#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace StrUtils
{
    // Whitespace
    std::string trim(std::string_view str);
    std::string trim_left(std::string_view str);
    std::string trim_right(std::string_view str);

    // Case conversion
    std::string to_lower(std::string_view str);
    std::string to_upper(std::string_view str);

    // Searching
    bool starts_with(std::string_view str, std::string_view prefix);
    bool ends_with(std::string_view str, std::string_view suffix);
    bool contains(std::string_view str, std::string_view value);

    // Splitting / Joining
    std::vector<std::string> split(const std::string &str, char delimiter);
    std::string join(const std::vector<std::string> &items, std::string_view delimiter);

    // Replacement
    std::string replace(std::string str, std::string_view from, std::string_view to);

    // Checks
    bool is_empty(std::string_view str);
    bool is_whitespace(std::string_view str);

    // Counting
    std::size_t count(std::string_view str, char character);

    // Padding
    std::string pad_left(std::string_view str, std::size_t length, char fill = ' ');

    std::string pad_right(std::string_view str, std::size_t length, char fill = ' ');

    // Remove
    std::string remove_whitespace(std::string_view str);

    // Repeating
    std::string repeat(std::string_view str, std::size_t times);

}