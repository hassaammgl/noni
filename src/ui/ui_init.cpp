#include "ui_impl.hpp"

void UI::init()
{
    setlocale(LC_ALL, "");
    initscr();

    raw();
    noecho();
    set_escdelay(config.esc_delay_ms > 0 ? config.esc_delay_ms : 25);
    signal(SIGINT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    termios term{};
    if (tcgetattr(STDIN_FILENO, &term) == 0)
    {
        term.c_iflag &= static_cast<tcflag_t>(~(IXON | IXOFF | IXANY));
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
    }

    use_extended_names(TRUE);
    keypad(stdscr, TRUE);
    meta(stdscr, TRUE);
    nonl(); // keep CR (Enter/Ctrl+M) distinct from LF (Ctrl+J)
    KeybindingEngine::register_extended_keys();
    timeout(50); // let async work (index/git/scan) refresh UI without keypress
    curs_set(0);

    if (has_colors())
        Theme::init();

    wbkgd(stdscr, COLOR_PAIR(Theme::Editor));
    erase();
    refresh();

    sidebar_width = config.sidebar_width;
    line_number_width = config.line_number_width;

    resize();
}

void UI::load_config()
{
    config = AppConfig::load();
    keys.load(config);
    GrammarInstaller::set_auto_install(config.syntax_auto_install);
    LspInstaller::set_auto_install(config.lsp_auto_install);

    std::vector<LspServerConfig> servers;
    std::vector<std::string> lsp_bins;
    servers.reserve(config.lsp_servers.size());
    for (const auto &s : config.lsp_servers)
    {
        LspServerConfig c;
        c.language = s.language;
        c.command = s.command;
        c.root_markers = s.root_markers;
        if (!c.command.empty())
            lsp_bins.push_back(c.command[0]);
        servers.push_back(std::move(c));
    }
    core.lsp().set_server_configs(std::move(servers));
    LspInstaller::set_workspace_root(project_root());
    LspInstaller::request_all(lsp_bins);

    terminal_height = std::max(5, config.terminal.height);
    TerminalSessionConfig tcfg;
    tcfg.shell = config.terminal.shell;
    tcfg.scrollback = std::max(0, config.terminal.scrollback);
    tcfg.cwd = project_root().string();
    terminal.apply_config(tcfg);
}
