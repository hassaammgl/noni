#pragma once

#include <configs/mini_json.hpp>
#include <string>
#include <vector>

struct Keybinding
{
    std::string key;
    std::string command;
    std::string when;
};

struct AppConfig
{
    std::string app_name = "noni";
    int sidebar_width = 25;
    int line_number_width = 5;
    int esc_delay_ms = 25;
    std::vector<Keybinding> keybindings;

    static AppConfig defaults();
    static AppConfig load(const std::string &path = "config.json");
};
