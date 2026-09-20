#pragma once

#include <configs/mini_json.hpp>
#include <string>
#include <map>
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
    bool lsp_auto_install = true;
    std::vector<Keybinding> keybindings;
    std::vector<LspServerConfigFile> lsp_servers;
    TerminalConfig terminal;
    // Namespaced extension settings: extensions.<id> → JSON object
    std::map<std::string, MiniJson::Value> extensions;

    // Set by load(): path used, or empty if none found.
    std::string loaded_from;
    // User-facing warning/error (missing file or parse fail). Empty if ok.
    std::string load_message;

    static AppConfig defaults();
    // cwd/config.json → exe dir → $HOME/.config/noni/config.json
    static AppConfig load();
    static AppConfig load_file(const std::string &path);
};
