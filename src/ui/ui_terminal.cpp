#include "ui_impl.hpp"

void UI::toggle_terminal()
{
    if (!terminal_visible)
    {
        open_terminal(true);
        return;
    }

    if (focus == Focus::Terminal)
    {
        close_terminal();
        return;
    }

    open_terminal(true);
}

void UI::open_terminal(bool focus_terminal)
{
    terminal_visible = true;
    terminal.set_visible(true);
    auto tcfg = terminal.session().config();
    tcfg.cwd = project_root().string();
    if (tcfg.shell.empty())
        tcfg.shell = config.terminal.shell;
    tcfg.scrollback = config.terminal.scrollback;
    terminal.apply_config(tcfg);
    terminal.set_cwd(project_root().string());
    keys.clear_chord();
    editor.enter_normal_mode();

    if (focus_terminal)
    {
        focus = Focus::Terminal;
        terminal.set_focused(true);
    }

    resize();
    terminal.ensure_started();
    terminal.on_resized();
}

void UI::close_terminal()
{
    terminal_visible = false;
    terminal.set_visible(false);
    terminal.set_focused(false);
    if (focus == Focus::Terminal)
        focus = Focus::Editor;
    keys.clear_chord();
    resize();
}

void UI::ex_terminal(const std::string &arg)
{
    const std::string a = StrUtils::to_lower(StrUtils::trim(arg));
    if (a.empty() || a == "open" || a == "toggle")
    {
        if (a == "toggle")
            toggle_terminal();
        else
            open_terminal(true);
        return;
    }
    if (a == "close" || a == "hide")
    {
        close_terminal();
        return;
    }
    if (a == "kill")
    {
        terminal.stop();
        Messages::info("Terminal process killed");
        return;
    }
    if (a == "clear")
    {
        if (!terminal_visible)
            open_terminal(false);
        terminal.clear_screen();
        return;
    }
    Messages::error("Usage: :terminal [open|close|toggle|kill|clear]");
}
