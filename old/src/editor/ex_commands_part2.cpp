#include <editor/ex_commands.hpp>

#include <ui/ui.hpp>
#include <utils/messages.hpp>
#include <utils/str.hpp>

#include <cctype>
#include <format>

#include "ex_commands_detail.hpp"

using namespace ex_commands_detail;

void ExCommands::register_ex_more()
{
    add({
        .name = "buffers",
        .aliases = {"ls", "filesopen"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":buffers",
        .description = "Fuzzy switch among open buffers/tabs",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_buffers();
        },
    });

    add({
        .name = "workspace",
        .aliases = {"ws", "cd"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 1,
        .usage = ":workspace [path]",
        .description = "Show or open a workspace/project root",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_workspace(cmd.args.empty() ? "" : cmd.args[0]);
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
        .usage = ":term[inal] [open|close|toggle|kill|clear]",
        .description = "Integrated terminal panel (PTY)",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_terminal(cmd.args.empty() ? "" : cmd.args[0]);
        },
    });

    add({
        .name = "undo",
        .aliases = {"u"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":u[ndo]",
        .description = "Undo last change",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_undo();
        },
    });

    add({
        .name = "redo",
        .aliases = {},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":redo",
        .description = "Redo last undone change",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_redo();
        },
    });

    add({
        .name = "commands",
        .aliases = {"com"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":com[mands]",
        .description = "Open help for all ex commands",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_help("ex");
        },
    });

    add({
        .name = "lsp",
        .aliases = {"LspInstall", "lsps"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 0,
        .usage = ":lsp",
        .description = "Open LSP install panel (Enter installs selected server)",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_lsp();
        },
    });

    add({
        .name = "help",
        .aliases = {"h"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 1,
        .usage = ":h[elp] [topic|command|all]",
        .description = "Full documentation (topics, keys, ex, LSP, terminal, …)",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_help(cmd.args.empty() ? "" : cmd.args[0]);
        },
    });
}
