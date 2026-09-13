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

struct LspServerConfigFile
{
    std::string language;
    std::vector<std::string> command;
    std::vector<std::string> root_markers;
};

struct TerminalConfig
{
    std::string shell; // empty → $SHELL → /bin/sh
    int height = 12;
    int scrollback = 5000;
};

struct AppConfig
{
    std::string app_name = "noni";
    int sidebar_width = 25;
    int line_number_width = 5;
    int esc_delay_ms = 25;
    bool syntax_auto_install = true;
    std::vector<Keybinding> keybindings;
    std::vector<LspServerConfigFile> lsp_servers;
    TerminalConfig terminal;

    static AppConfig defaults();
    static AppConfig load(const std::string &path = "config.json");
};
