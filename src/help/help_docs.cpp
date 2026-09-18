#include <help/help_docs.hpp>
#include <utils/str.hpp>

#include <algorithm>
#include <cctype>
#include <format>

namespace
{
    void line(std::vector<std::string> &out, std::string s)
    {
        out.push_back(std::move(s));
    }

    void blank(std::vector<std::string> &out) { out.emplace_back(""); }

    void section(std::vector<std::string> &out, const std::string &title)
    {
        blank(out);
        line(out, std::format("=== {} ===", title));
        blank(out);
    }

    void bullet(std::vector<std::string> &out, const std::string &k, const std::string &v)
    {
        line(out, std::format("  {:<28} {}", k, v));
    }

    std::string lower(std::string s)
    {
        for (char &c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    void page_index(std::vector<std::string> &out)
    {
        section(out, "NONI HELP");
        line(out, "  Terminal editor - keyboard-first. Esc closes this panel.");
        blank(out);
        line(out, "Usage:");
        line(out, "  :help              this index");
        line(out, "  :help <topic>      topic page");
        line(out, "  :help <ex-cmd>     ex-command usage");
        line(out, "  :help all          dump every topic");
        line(out, "  :commands          list ex commands only");
        blank(out);
        line(out, "Topics:");
        for (const auto &id : HelpDocs::topic_ids())
            line(out, std::format("  :help {}", id));
    }

    void page_modes(std::vector<std::string> &out)
    {
        section(out, "MODES");
        bullet(out, "Normal", "navigation, operators, Space-leader chords");
        bullet(out, "Insert", "type text (i/a/o/O/c...). Esc -> Normal");
        bullet(out, "Visual", "character selection (v)");
        bullet(out, "Visual-Line", "line selection (V)");
        blank(out);
        line(out, "Esc / noni.mode.normal returns focus to the editor in Normal mode.");
    }

    void page_motions(std::vector<std::string> &out)
    {
        section(out, "MOTIONS (Normal / Visual)");
        bullet(out, "h j k l / arrows", "left / down / up / right");
        bullet(out, "w b e", "word forward / back / end");
        bullet(out, "0 ^ $", "line start / first non-blank / line end");
        bullet(out, "gg G", "file start / file end");
        bullet(out, "f{c} F{c}", "find char forward / back on line");
        bullet(out, "t{c} T{c}", "till char forward / back");
        bullet(out, "; ,", "repeat last f/t / reverse");
        bullet(out, "PgUp / PgDn", "page scroll");
        bullet(out, "Ctrl+u / Ctrl+d", "half-page (where supported)");
        blank(out);
        line(out, "Motions feed operators (d/c/y) and extend Visual selections.");
    }

    void page_operators(std::vector<std::string> &out)
    {
        section(out, "OPERATORS & EDITING");
        bullet(out, "d{motion} / dd", "delete");
        bullet(out, "c{motion} / cc", "change (delete + Insert)");
        bullet(out, "y{motion} / yy", "yank");
        bullet(out, "x / X", "delete char under / before cursor");
        bullet(out, "p / P", "paste after / before");
        bullet(out, "u / Ctrl+r", "undo / redo");
        bullet(out, "i a o O", "insert / append / open line");
        bullet(out, "\"{reg}", "select register before yank/paste");
        bullet(out, "m{a-z}", "set mark");
        bullet(out, "'{a-z}", "jump to mark");
        blank(out);
        line(out, "Clipboard paste: Space p  (wl-paste / xclip).");
    }

    void page_search(std::vector<std::string> &out)
    {
        section(out, "SEARCH & REPLACE");
        bullet(out, "/pattern  ?pattern", "forward / backward search in buffer");
        bullet(out, "n  N", "next / previous match");
        bullet(out, ":search", "project find-in-files sidebar");
        bullet(out, "Space s", "same as :search");
        blank(out);
        line(out, "In-buffer search state is per EditorCore; highlights paint under syntax.");
    }

    void page_windows(std::vector<std::string> &out)
    {
        section(out, "WINDOWS / SPLITS");
        bullet(out, "Ctrl+w v", "split vertical");
        bullet(out, "Ctrl+w s", "split horizontal");
        bullet(out, "Ctrl+w q", "close active split");
        bullet(out, "Ctrl+w h/j/k/l", "focus left/down/up/right");
        bullet(out, "Ctrl+w </>/-/+", "resize width / height");
        blank(out);
        line(out, "Splits share Buffers; each Window has its own cursor/scroll/selection.");
    }

    void page_buffers(std::vector<std::string> &out)
    {
        section(out, "BUFFERS / TABS");
        bullet(out, ":e[dit] [file]", "open file");
        bullet(out, ":bn[ext]  :bp", "next / previous tab");
        bullet(out, ":bd[elete][!]", "close tab");
        bullet(out, "g t / g T", "next / previous tab");
        bullet(out, "Space ; / :buffers / :ls", "fuzzy open-buffer picker");
        bullet(out, "Space x", "close active editor");
        bullet(out, "Space w / :w", "save");
        bullet(out, ":wq / :q[!]", "write-quit / quit");
        bullet(out, "Space f / :find", "fuzzy file quick-open (recent when empty)");
    }

    void page_workspace(std::vector<std::string> &out)
    {
        section(out, "WORKSPACE / NAVIGATION");
        bullet(out, "Space o / :workspace [path]", "open or show project root");
        bullet(out, "Space f / :find", "fuzzy files under workspace (+ recent)");
        bullet(out, "Space ; / :buffers", "switch among open tabs");
        bullet(out, "Space .", "reveal active file in explorer");
        blank(out);
        line(out, "Workspace root is EditorCore-owned (not Window-local).");
        line(out, "Opening a directory argv sets workspace and an untitled buffer.");
        line(out, "Rename remaps Buffer save path; delete closes matching buffers.");
        line(out, "Filesystem watcher (inotify): auto-reloads clean buffers;");
        line(out, "dirty buffers warn on statusbar. Explorer refreshes on tree changes.");
        line(out, "Logger output goes only to logs/noni.log (not the UI).");
    }

    void page_sidebar(std::vector<std::string> &out)
    {
        section(out, "SIDEBAR / EXPLORER");
        bullet(out, "Tab", "toggle focus Editor <-> Sidebar");
        bullet(out, "Space b", "toggle sidebar visibility");
        bullet(out, "Space e", "explorer view / focus editor from sidebar");
        bullet(out, "Space .", "reveal active file");
        bullet(out, ":sidebar ...", "open|close|toggle");
        blank(out);
        line(out, "Explorer supports open, create, rename, delete (with prompts).");
    }

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

    void page_lsp(std::vector<std::string> &out)
    {
        section(out, "LSP");
        bullet(out, "Space l s", "lsp.showStatus");
        bullet(out, "Space l r", "lsp.restart");
        bullet(out, "Ctrl+Space / Space l c", "trigger completion");
        bullet(out, "g d / g D / g y", "definition / declaration / type definition");
        bullet(out, "g r", "find references");
        bullet(out, "Space l o / Space l w", "document / workspace symbols");
        bullet(out, "Space l n", "rename symbol");
        bullet(out, "Space l a", "code actions / quick fix");
        blank(out);
        line(out, "Configured in config.json -> lsp.servers (language, command, rootMarkers).");
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
        bullet(out, "syntaxAutoInstall", "tree-sitter: curl+tar (nvim-style) then git");
        bullet(out, "terminal.*", "shell, height, scrollback");
        bullet(out, "lsp.servers[]", "language servers");
        bullet(out, "extensions.*", "per-extension settings");
        bullet(out, "keybindings[]", "key / command / when");
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
        bullet(out, ":help all", "full documentation dump");
        blank(out);
        line(out, "Coordinate systems: Cursor.column = UTF-8 bytes;");
        line(out, "display columns for scroll; UTF-16 only at LSP boundary.");
    }
}

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
