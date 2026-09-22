#include <help/help_docs.hpp>
#include "help_pages.hpp"
#include <utils/str.hpp>

using namespace help_detail;

std::vector<std::string> HelpDocs::topic_ids()
{
    return {
        "index",
        "modes",
        "motions",
        "operators",
        "search",
        "windows",
        "buffers",
        "workspace",
        "sidebar",
        "git",
        "session",
        "lsp",
        "terminal",
        "extensions",
        "config",
        "keys",
        "ex",
        "all",
    };
}

bool HelpDocs::build(
    std::string_view topic,
    const AppConfig &config,
    const std::vector<ExCommand> &ex_commands,
    std::vector<std::string> &out_lines,
    std::string &error)
{
    out_lines.clear();
    error.clear();

    std::string t = lower(std::string(StrUtils::trim(topic)));
    if (t.empty() || t == "index" || t == "help")
    {
        page_index(out_lines);
        page_overview_extras(out_lines);
        return true;
    }

    if (t == "all")
    {
        page_index(out_lines);
        page_overview_extras(out_lines);
        page_modes(out_lines);
        page_motions(out_lines);
        page_operators(out_lines);
        page_search(out_lines);
        page_windows(out_lines);
        page_buffers(out_lines);
        page_workspace(out_lines);
        page_sidebar(out_lines);
        page_git(out_lines);
        page_session(out_lines);
        page_lsp(out_lines);
        page_terminal(out_lines);
        page_extensions(out_lines);
        page_config(out_lines);
        page_keybindings(out_lines, config);
        page_ex(out_lines, ex_commands);
        blank(out_lines);
        line(out_lines, "-- end of :help all --");
        return true;
    }

    if (t == "modes" || t == "mode")
    {
        page_modes(out_lines);
        return true;
    }
    if (t == "motions" || t == "motion" || t == "move")
    {
        page_motions(out_lines);
        return true;
    }
    if (t == "operators" || t == "operator" || t == "edit" || t == "editing")
    {
        page_operators(out_lines);
        return true;
    }
    if (t == "search" || t == "find" || t == "replace")
    {
        page_search(out_lines);
        return true;
    }
    if (t == "windows" || t == "window" || t == "splits" || t == "split")
    {
        page_windows(out_lines);
        return true;
    }
    if (t == "buffers" || t == "buffer" || t == "tabs" || t == "tab")
    {
        page_buffers(out_lines);
        return true;
    }
    if (t == "workspace" || t == "project" || t == "navigation" || t == "nav")
    {
        page_workspace(out_lines);
        return true;
    }
    if (t == "sidebar" || t == "explorer")
    {
        page_sidebar(out_lines);
        return true;
    }
    if (t == "git" || t == "scm")
    {
        page_git(out_lines);
        return true;
    }
    if (t == "session" || t == "recovery" || t == "persist" || t == "persistence")
    {
        page_session(out_lines);
        return true;
    }
    if (t == "lsp" || t == "completion" || t == "diagnostics")
    {
        page_lsp(out_lines);
        return true;
    }
    if (t == "terminal" || t == "term" || t == "pty")
    {
        page_terminal(out_lines);
        return true;
    }
    if (t == "extensions" || t == "extension" || t == "plugins" || t == "plugin")
    {
        page_extensions(out_lines);
        return true;
    }
    if (t == "config" || t == "configuration" || t == "settings")
    {
        page_config(out_lines);
        return true;
    }
    if (t == "keys" || t == "keybindings" || t == "keymap" || t == "bindings")
    {
        page_keybindings(out_lines, config);
        return true;
    }
    if (t == "ex" || t == "commands" || t == "cmd")
    {
        page_ex(out_lines, ex_commands);
        return true;
    }

    // Fall back: treat as ex-command name.
    ExCommands::instance().ensure_registered();
    const ExCommand *found = ExCommands::instance().resolve(t);
    if (found)
    {
        section(out_lines, "EX COMMAND");
        line(out_lines, std::format("  {}", found->usage));
        line(out_lines, std::format("  {}", found->description));
        if (!found->aliases.empty())
        {
            std::string a;
            for (std::size_t i = 0; i < found->aliases.size(); ++i)
            {
                if (i)
                    a += ", ";
                a += found->aliases[i];
            }
            line(out_lines, std::format("  aliases: {}", a));
        }
        line(out_lines, std::format("  bang allowed: {}", found->bang_allowed ? "yes" : "no"));
        blank(out_lines);
        line(out_lines, "  See also: :help ex   :help index");
        return true;
    }

    error = std::format("E149: No help for {}", std::string(topic));
    return false;
}
