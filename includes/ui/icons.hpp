#pragma once

#include <ui/icons_workbench.hpp>
#include <ui/icons_chrome.hpp>
#include <ui/icons_lang.hpp>
#include <ui/icons_lang2.hpp>
#include <ui/icons_lang3.hpp>
#include <ui/icons_lang4.hpp>

namespace Icons
{
    inline std::string lower_ascii(std::string value)
    {
        for (char &c : value)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return value;
    }

    inline bool ends_with(const std::string &value, const std::string &suffix)
    {
        return value.size() >= suffix.size() &&
               value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    inline bool starts_with(const std::string &value, const std::string &prefix)
    {
        return value.size() >= prefix.size() &&
               value.compare(0, prefix.size(), prefix) == 0;
    }

    const wchar_t *for_file(const fs::path &path);
}
