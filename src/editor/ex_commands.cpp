#include <editor/ex_commands.hpp>

#include <ui/ui.hpp>
#include <utils/messages.hpp>
#include <utils/str.hpp>

#include <cctype>
#include <format>

namespace
{
    bool is_cmd_char(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    std::vector<std::string> split_args(std::string_view raw)
    {
        std::vector<std::string> args;
        std::string cur;
        bool in_quotes = false;

        for (std::size_t i = 0; i < raw.size(); ++i)
        {
            const char c = raw[i];
            if (c == '"')
            {
                in_quotes = !in_quotes;
                continue;
            }
            if (!in_quotes && std::isspace(static_cast<unsigned char>(c)))
            {
                if (!cur.empty())
                {
                    args.push_back(cur);
                    cur.clear();
                }
                continue;
            }
            cur.push_back(c);
        }

        if (!cur.empty())
            args.push_back(cur);

        return args;
    }
}

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

    add({
        .name = "quit",
        .aliases = {"q"},
        .bang_allowed = true,
        .min_args = 0,
        .max_args = 0,
        .usage = ":q[uit][!]",
        .description = "Quit / close current buffer",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_quit(cmd.bang);
        },
    });

    add({
        .name = "write",
        .aliases = {"w"},
        .bang_allowed = true,
        .min_args = 0,
        .max_args = 1,
        .usage = ":w[rite][!] [file]",
        .description = "Write current buffer to disk",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_write(cmd.bang, cmd.args.empty() ? "" : cmd.args[0]);
        },
    });

    add({
        .name = "wq",
        .aliases = {"x"},
        .bang_allowed = true,
        .min_args = 0,
        .max_args = 1,
        .usage = ":wq[!] [file]",
        .description = "Write and quit",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_write_quit(cmd.bang, cmd.args.empty() ? "" : cmd.args[0]);
        },
    });

    add({
        .name = "edit",
        .aliases = {"e"},
        .bang_allowed = true,
        .min_args = 0,
        .max_args = 1,
        .usage = ":e[dit][!] [file]",
        .description = "Edit a file in a new/existing tab",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_edit(cmd.bang, cmd.args.empty() ? "" : cmd.args[0]);
        },
    });

    add({
        .name = "bnext",
        .aliases = {"bn"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":bn[ext]",
        .description = "Go to next buffer tab",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_bnext();
        },
    });

    add({
        .name = "bprevious",
        .aliases = {"bp", "bprev"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":bp[revious]",
        .description = "Go to previous buffer tab",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_bprevious();
        },
    });

    add({
        .name = "bdelete",
        .aliases = {"bd"},
        .bang_allowed = true,
        .min_args = 0,
        .max_args = 0,
        .usage = ":bd[elete][!]",
        .description = "Delete/close current buffer tab",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_bdelete(cmd.bang);
        },
    });

    add({
        .name = "messages",
        .aliases = {"mes"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":mes[sages]",
        .description = "Show message history",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_messages();
        },
    });

    add({
        .name = "find",
        .aliases = {"Files", "files"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":find",
        .description = "Fuzzy file search (fzf-style quick open)",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_find();
        },
    });

    add({
        .name = "sidebar",
        .aliases = {"sb"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 1,
        .usage = ":sidebar [open|close|toggle]",
        .description = "Show, hide, or toggle the file explorer sidebar",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_sidebar(cmd.args.empty() ? "" : cmd.args[0]);
        },
    });

    add({
        .name = "search",
        .aliases = {"grep"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":search",
        .description = "Open VS Code-style search sidebar (find in files)",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_search();
        },
    });

    add({
        .name = "terminal",
        .aliases = {"term"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 1,
        .usage = ":term[inal] [open|close|toggle]",
        .description = "Integrated terminal panel",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_terminal(cmd.args.empty() ? "" : cmd.args[0]);
        },
    });

    add({
        .name = "commands",
        .aliases = {"com"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":com[mands]",
        .description = "List available ex commands",
        .run =
            [](UI &, const ParsedEx &)
        {
            for (const auto &c : ExCommands::instance().all())
            {
                Messages::info(std::format("{}  —  {}", c.usage, c.description));
            }
        },
    });

    add({
        .name = "help",
        .aliases = {"h"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 1,
        .usage = ":h[elp] [command]",
        .description = "Show command usage template",
        .run =
            [](UI &, const ParsedEx &cmd)
        {
            if (cmd.args.empty())
            {
                Messages::info("Usage: :h[elp] {command}  |  :com[mands]");
                return;
            }

            const ExCommand *found = ExCommands::instance().resolve(cmd.args[0]);
            if (!found)
            {
                Messages::error(std::format("E149: No help for {}", cmd.args[0]));
                return;
            }

            Messages::info(std::format("{}  —  {}", found->usage, found->description));
        },
    });
}
