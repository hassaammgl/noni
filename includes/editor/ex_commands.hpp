#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

class UI;

struct ParsedEx
{
    std::string name;
    bool bang = false;
    std::vector<std::string> args;
    std::string raw_args;
};

struct ExCommand
{
    std::string name;
    std::vector<std::string> aliases;
    bool bang_allowed = false;
    int min_args = 0;
    int max_args = 0; // -1 = unlimited
    std::string usage; // neovim-style template, e.g. ":w[rite][!] [file]"
    std::string description;
    std::function<void(UI &ui, const ParsedEx &cmd)> run;
};

class ExCommands
{
public:
    static ExCommands &instance();

    void ensure_registered();
    bool execute(UI &ui, std::string_view line);

    const std::vector<ExCommand> &all() const;

    static ParsedEx parse(std::string_view line);
    const ExCommand *resolve(std::string_view name) const;

private:
    ExCommands() = default;

    void register_builtins();
    void add(ExCommand cmd);

    bool registered = false;
    std::vector<ExCommand> commands;
};
