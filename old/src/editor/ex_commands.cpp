#include <editor/ex_commands.hpp>

#include <ui/ui.hpp>
#include <utils/messages.hpp>
#include <utils/str.hpp>

#include <cctype>
#include <format>

#include "ex_commands_detail.hpp"

using namespace ex_commands_detail;

ExCommands &ExCommands::instance()
{
    static ExCommands engine;
    return engine;
}

void ExCommands::ensure_registered()
{
    if (!registered)
        register_builtins();
}

void ExCommands::add(ExCommand cmd)
{
    commands.push_back(std::move(cmd));
}

const std::vector<ExCommand> &ExCommands::all() const
{
    return commands;
}

ParsedEx ExCommands::parse(std::string_view line)
{
    ParsedEx out;
    std::string s = StrUtils::trim(line);
    if (s.empty())
        return out;

    // Skip optional range tokens like %, ., 1,10 for future use.
    std::size_t i = 0;
    if (s[0] == '%' || s[0] == '.' || s[0] == '$' || (s[0] >= '0' && s[0] <= '9'))
    {
        while (i < s.size() && !is_cmd_char(s[i]) && s[i] != '!')
            ++i;
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
            ++i;
    }

    while (i < s.size() && is_cmd_char(s[i]))
    {
        out.name.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(s[i]))));
        ++i;
    }

    if (i < s.size() && s[i] == '!')
    {
        out.bang = true;
        ++i;
    }

    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
        ++i;

    out.raw_args = s.substr(i);
    out.args = split_args(out.raw_args);
    return out;
}

const ExCommand *ExCommands::resolve(std::string_view name) const
{
    if (name.empty())
        return nullptr;

    const ExCommand *exact = nullptr;
    std::vector<const ExCommand *> prefixes;

    for (const auto &cmd : commands)
    {
        auto matches = [&](std::string_view candidate)
        {
            if (candidate == name)
                return 2;
            if (StrUtils::starts_with(candidate, name))
                return 1;
            return 0;
        };

        if (matches(cmd.name) == 2)
            exact = &cmd;
        else if (matches(cmd.name) == 1)
            prefixes.push_back(&cmd);

        for (const auto &alias : cmd.aliases)
        {
            if (matches(alias) == 2)
                exact = &cmd;
            else if (matches(alias) == 1)
                prefixes.push_back(&cmd);
        }
    }

    if (exact)
        return exact;

    // Unique abbreviation like Neovim.
    if (prefixes.size() == 1)
        return prefixes[0];

    return nullptr;
}

bool ExCommands::execute(UI &ui, std::string_view line)
{
    ensure_registered();

    const ParsedEx parsed = parse(line);
    if (parsed.name.empty())
        return true;

    const ExCommand *cmd = resolve(parsed.name);
    if (!cmd)
    {
        Messages::error(std::format("E492: Not an editor command: {}", parsed.name));
        return false;
    }

    if (parsed.bang && !cmd->bang_allowed)
    {
        Messages::error(std::format("E477: No ! allowed: {}", cmd->usage));
        return false;
    }

    const int argc = static_cast<int>(parsed.args.size());
    if (argc < cmd->min_args || (cmd->max_args >= 0 && argc > cmd->max_args))
    {
        Messages::error(std::format("E488: Trailing characters / wrong args. Usage: {}", cmd->usage));
        return false;
    }

    cmd->run(ui, parsed);
    return true;
}

void ExCommands::register_builtins()
{
    registered = true;
    commands.clear();
    register_ex_core();
    register_ex_more();
}
