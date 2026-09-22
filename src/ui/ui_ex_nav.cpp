#include "ui_impl.hpp"

void UI::ex_bnext()
{
    core.buffers().next_tab();
    sync_active_tab();
}

void UI::ex_bprevious()
{
    core.buffers().prev_tab();
    sync_active_tab();
}

void UI::ex_bdelete(bool bang)
{
    close_active_tab(bang);
}

void UI::ex_messages()
{
    messages_panel.open("Messages");
    focus = Focus::Messages;
}

void UI::ex_logs(const std::string &which)
{
    struct Entry
    {
        const char *key;
        const char *path;
        const char *title;
    };
    static constexpr Entry kLogs[] = {
        {"noni", "logs/noni.log", "noni.log (app Logger)"},
        {"lsp", "logs/lsp.stderr.log", "lsp.stderr.log (clangd/pylsp stderr)"},
        {"install", "logs/lsp-install.log", "lsp-install.log"},
        {"grammar", "logs/grammar-install.log", "grammar-install.log"},
    };

    const std::string w = StrUtils::to_lower(StrUtils::trim(which));
    std::vector<std::string> out;
    out.push_back("LOGS — file-backed (not the statusbar Messages ring)");
    out.push_back("  :logs          → all (last ~80 lines each)");
    out.push_back("  :logs noni|lsp|install|grammar");
    out.push_back("  Tip: :messages only shows Messages::info/warning, not Logger.");
    out.push_back("");

    auto append_file = [&](const Entry &e) {
        out.push_back(std::format("═══ {} ({}) ═══", e.title, e.path));
        std::ifstream in(e.path);
        if (!in)
        {
            out.push_back(std::format("  (missing — nothing written yet)"));
            out.push_back("");
            return;
        }
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line))
            lines.push_back(std::move(line));
        const std::size_t keep = 80;
        const std::size_t start =
            lines.size() > keep ? lines.size() - keep : 0;
        for (std::size_t i = start; i < lines.size(); ++i)
            out.push_back(lines[i]);
        out.push_back("");
    };

    bool any = false;
    for (const auto &e : kLogs)
    {
        if (!w.empty() && w != "all" && w != e.key)
            continue;
        append_file(e);
        any = true;
    }
    if (!any)
    {
        Messages::error("E149: Unknown log — use noni|lsp|install|grammar");
        return;
    }

    Messages::set_lines(std::move(out));
    messages_panel.open(w.empty() || w == "all" ? "Logs" : std::format("Logs: {}", w));
    focus = Focus::Messages;
    keys.clear_chord();
}

void UI::ex_help(const std::string &topic)
{
    ExCommands::instance().ensure_registered();
    std::vector<std::string> lines;
    std::string err;
    if (!HelpDocs::build(topic, config, ExCommands::instance().all(), lines, err))
    {
        Messages::error(err.empty() ? "E149: No help for topic" : err);
        return;
    }
    Messages::set_lines(std::move(lines));
    messages_panel.open(topic.empty() || topic == "index" ? "Help" : std::format("Help: {}", topic));
    focus = Focus::Messages;
    keys.clear_chord();
}

void UI::ex_sidebar(const std::string &arg)
{
    const std::string a = StrUtils::to_lower(StrUtils::trim(arg));
    if (a.empty() || a == "toggle")
        toggle_sidebar();
    else if (a == "open" || a == "show" || a == "on")
        open_sidebar(true);
    else if (a == "close" || a == "hide" || a == "off")
        close_sidebar();
    else
        Messages::error("Usage: :sidebar [open|close|toggle]");
}

void UI::ex_find()
{
    open_file_search();
}

void UI::ex_buffers()
{
    open_buffer_search();
}

void UI::ex_workspace(const std::string &arg)
{
    const std::string a = StrUtils::trim(arg);
    if (a.empty())
    {
        if (core.workspace().has_root())
            Messages::info(std::format("Workspace: {}", core.workspace().root().string()));
        else
            Messages::info("No workspace root");
        return;
    }
    apply_workspace_root(fs::path(a), true);
}

void UI::ex_search()
{
    open_search_view(true);
}

void UI::ex_undo()
{
    if (!core.buffers().has_tabs())
        return;
    if (!editor.undo())
        Messages::info("Nothing to undo");
}

void UI::ex_redo()
{
    if (!core.buffers().has_tabs())
        return;
    if (!editor.redo())
        Messages::info("Nothing to redo");
}
