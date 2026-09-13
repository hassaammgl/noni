#pragma once

#include <configs/config.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct KeyToken
{
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    int code = 0; // printable char, or special: 27 esc, 9 tab, 10 enter, etc.

    bool operator==(const KeyToken &o) const
    {
        return ctrl == o.ctrl && alt == o.alt && shift == o.shift && code == o.code;
    }
};

struct ResolvedBinding
{
    std::vector<KeyToken> chord;
    std::string command;
    std::string when;
};

class KeybindingEngine
{
public:
    void load(const AppConfig &config);
    void register_command(const std::string &id, std::function<void()> action);

    // Returns true if a binding fully matched and ran (or chord advanced).
    bool handle(int raw_key, const std::string &when_context);

    void clear_chord();

    static KeyToken from_raw(int raw_key);
    static std::vector<KeyToken> parse_key(const std::string &spec);
    static bool when_matches(const std::string &when, const std::string &context);

private:
    std::vector<ResolvedBinding> bindings;
    std::unordered_map<std::string, std::function<void()>> actions;
    std::vector<KeyToken> pending;
};
