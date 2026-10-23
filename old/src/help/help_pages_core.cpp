#include "help_pages.hpp"
#include <help/help_docs.hpp>

namespace help_detail
{
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
        bullet(out, "u / Ctrl+y", "undo / redo");
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
        bullet(out, "Ctrl+Shift+f / Space Shift+f", "find in files");
        bullet(out, "Ctrl+p", "quick open files");
        blank(out);
        line(out, "In-buffer search state is per EditorCore; highlights paint under syntax.");
    }

    void page_windows(std::vector<std::string> &out)
    {
        section(out, "WINDOWS / SPLITS");
        bullet(out, "Ctrl+\\", "split vertical");
        bullet(out, "Ctrl+k s", "split horizontal");
        bullet(out, "Ctrl+k q", "close active split");
        bullet(out, "Ctrl+k h/j/k/l", "focus left/down/up/right");
        bullet(out, "Ctrl+k [ ] _ =", "resize width / height");
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
        bullet(out, "Ctrl+s / :w", "save");
        bullet(out, "Ctrl+w", "close tab");
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

}
