#include "help_pages.hpp"

namespace help_detail
{
    void page_git(std::vector<std::string> &out)
    {
        section(out, "GIT / SCM");
        bullet(out, "Space g r", "git.refresh - async status refresh");
        bullet(out, "Space g s", "git.showStatus - branch, ahead/behind, XY");
        bullet(out, "Space g a", "git.stage - stage active file");
        bullet(out, "Space g u", "git.unstage - unstage active file");
        bullet(out, "Space g x", "git.discard - discard worktree (confirm)");
        bullet(out, "Space g d", "git.showDiff - line change summary");
        bullet(out, "Space g f", "git.refreshDiff - refresh gutter diff");
        blank(out);
        line(out, "Header shows branch; statusbar can show file XY badge.");
        line(out, "Gutter: green=added, blue=modified, red dash=deleted (vs HEAD).");
        line(out, "Discard refuses if Buffer is dirty (unsaved). No merge/rebase UI.");
        line(out, "Tab ● = unsaved buffer; sidebar • = currently open file (not git).");
    }

    void page_session(std::vector<std::string> &out)
    {
        section(out, "SESSION / RECOVERY");
        bullet(out, "Space m s", "saveSession — write .noni/session.json");
        bullet(out, "Space m r", "restoreSession — reopen tabs from session");
        blank(out);
        line(out, "On quit: session (workspace, tabs, cursors, recent, UI flags) is saved.");
        line(out, "On start (no file arg): session tabs are restored if present.");
        line(out, "Dirty buffers: snapshots under .noni/recovery/ (never overwrite sources).");
        line(out, "Crash → next launch prompts Recover? for each snapshot.");
        line(out, "Successful :w / save clears that file's recovery snapshot.");
    }

    void page_lsp(std::vector<std::string> &out)
    {
        section(out, "LSP");
        bullet(out, "Space l s", "lsp.showStatus");
        bullet(out, "Space l r", "lsp.restart");
        bullet(out, ":lsp", "install panel (Enter installs selected server)");
        bullet(out, "Ctrl+Space / Space l c", "trigger completion");
        bullet(out, ":logs [noni|lsp|install|grammar]", "file logs under logs/");
        bullet(out, "F12 / g d", "go to definition");
        bullet(out, "F2 / Space l n", "rename");
        bullet(out, "g d / g D / g y", "definition / declaration / type definition");
        bullet(out, "g r", "find references");
        bullet(out, "Space l o / Space l w", "document / workspace symbols");
        bullet(out, "Space l n", "rename symbol");
        bullet(out, "Space l a", "code actions / quick fix");
        blank(out);
        line(out, "Configured in config.json -> lsp.servers (language, command, rootMarkers).");
        line(out, "Logger → logs/noni.log (truncated each launch). LSP stderr → logs/lsp.stderr.log.");
        line(out, ":messages = statusbar ring only; :logs = file logs.");
        line(out, "Diagnostics: gutter markers (! ? i .) + inline highlight.");
        line(out, "Completion picker: ^v, Enter/Tab accept, Esc cancel.");
        line(out, "Navigation results use the shared LSP picker; jumps recorded.");
        line(out, "WorkspaceEdit (rename/actions) applies with Buffer undo transactions.");
        line(out, "Tree-sitter remains syntax highlighting authority (not LSP tokens).");
    }

    void page_terminal(std::vector<std::string> &out)
    {
        section(out, "TERMINAL");
        bullet(out, "Space t", "toggle integrated terminal");
        bullet(out, ":terminal ...", "open|close|toggle|kill|clear");
        bullet(out, "Esc / Ctrl+]", "leave terminal focus (shell may keep running)");
        bullet(out, "PgUp / PgDn", "scrollback");
        blank(out);
        line(out, "Real PTY + VT parser (cells, SGR, cursor, bounded scrollback).");
        line(out, "config.json -> terminal.shell / height / scrollback.");
        line(out, "Shell: configured -> $SHELL -> /bin/sh. Cwd = project root.");
    }

    void page_extensions(std::vector<std::string> &out)
    {
        section(out, "EXTENSIONS");
        bullet(out, "Space h", "extension.hello.ping (sample)");
        bullet(out, "extension.showStatus", "loaded/active summary");
        blank(out);
        line(out, "Built-in/static extensions only (no marketplace / dlopen).");
        line(out, "Extensions register commands + subscribe to buffer events.");
        line(out, "Config: extensions.<id> { ... } in config.json.");
    }

    void page_config(std::vector<std::string> &out)
    {
        section(out, "CONFIGURATION");
        line(out, "File: config.json (project root)");
        blank(out);
        bullet(out, "sidebarWidth", "explorer width");
        bullet(out, "lineNumberWidth", "gutter width");
        bullet(out, "escDelayMs", "ncurses ESC delay");
        bullet(out, "syntaxAutoInstall", "tree-sitter → ~/.local/share/noni/tree-sitter/ (else .noni/tree-sitter)");
        line(out, "Statusbar shows download/compile path; details in logs/grammar-install.log");
        bullet(out, "lsp.autoInstall", "missing servers → ~/.local/share/noni/lsp/ (pip/npm/bun/curl/go)");
        line(out, "Details: logs/lsp-install.log — no sudo / no manual pip");
        bullet(out, "terminal.*", "shell, height, scrollback");
        bullet(out, "lsp.servers[]", "language servers");
        bullet(out, "extensions.*", "per-extension settings");
        bullet(out, "keybindings[]", "ONLY source of command keys (C++ defaults empty)");
        line(out, "Edit config.json — workbench/LSP chords are not hardcoded in code.");
        blank(out);
        line(out, "`when` context flags: editorFocus, normalMode, insertMode, visualMode,");
        line(out, "sidebarFocus, terminalFocus, searchFocus, ...");
    }

    void page_keybindings(std::vector<std::string> &out, const AppConfig &config)
    {
        section(out, "KEYBINDINGS (from config.json)");
        line(out, std::format("  {} binding(s) loaded", config.keybindings.size()));
        blank(out);
        for (const auto &kb : config.keybindings)
        {
            std::string when = kb.when.empty() ? "(always)" : kb.when;
            line(out, std::format("  {:<22} -> {}", kb.key, kb.command));
            line(out, std::format("    when: {}", when));
        }
    }

    void page_ex(std::vector<std::string> &out, const std::vector<ExCommand> &cmds)
    {
        section(out, "EX COMMANDS");
        line(out, "  Open with : in Normal mode. Bang (!) where allowed.");
        blank(out);
        for (const auto &c : cmds)
        {
            line(out, std::format("  {}", c.usage));
            line(out, std::format("    {}", c.description));
            if (!c.aliases.empty())
            {
                std::string a;
                for (std::size_t i = 0; i < c.aliases.size(); ++i)
                {
                    if (i)
                        a += ", ";
                    a += c.aliases[i];
                }
                line(out, std::format("    aliases: {}", a));
            }
        }
    }

    void page_overview_extras(std::vector<std::string> &out)
    {
        section(out, "QUICK START");
        bullet(out, ":", "ex command line");
        bullet(out, "Space", "leader (Normal / Visual / Sidebar...)");
        bullet(out, "/  ?", "search");
        bullet(out, "Space t", "terminal");
        bullet(out, "Space f", "fuzzy files");
        bullet(out, "Ctrl+Space", "LSP completion");
        bullet(out, "Ctrl+t", "toggle terminal");
        bullet(out, "Ctrl+p", "quick open");
        bullet(out, "Ctrl+s", "save");
        bullet(out, ":help all", "full documentation dump");
        blank(out);
        line(out, "Coordinate systems: Cursor.column = UTF-8 bytes;");
        line(out, "display columns for scroll; UTF-16 only at LSP boundary.");
    }
}
