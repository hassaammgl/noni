#include <editor/ex_commands.hpp>

#include <ui/ui.hpp>
#include <utils/messages.hpp>
#include <utils/str.hpp>

#include <cctype>
#include <format>

#include "ex_commands_detail.hpp"

using namespace ex_commands_detail;


void ExCommands::register_ex_core()
{

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
        .description = "Show statusbar Messages ring (not file logs)",
        .run =
            [](UI &ui, const ParsedEx &)
        {
            ui.ex_messages();
        },
    });

    add({
        .name = "logs",
        .aliases = {"log"},
        .bang_allowed = false,
        .min_args = 0,
        .max_args = 1,
        .usage = ":log[s] [noni|lsp|install|grammar]",
        .description = "View file logs (logs/*.log) in the messages panel",
        .run =
            [](UI &ui, const ParsedEx &cmd)
        {
            ui.ex_logs(cmd.args.empty() ? "" : cmd.args[0]);
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

}
