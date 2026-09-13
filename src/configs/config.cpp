#include <configs/config.hpp>
#include <utils/fs.hpp>
#include <utils/logger.hpp>

#include <format>
#include <fstream>
#include <sstream>

AppConfig AppConfig::defaults()
{
    AppConfig cfg;
    cfg.keybindings = {
        {"escape", "noni.mode.normal", ""},
        {"space w", "workbench.action.files.save", "editorFocus && normalMode"},
        {"space x", "workbench.action.closeActiveEditor", "editorFocus && normalMode"},
        {"space f", "noni.search.files", "editorFocus && normalMode || sidebarFocus || searchFocus"},
        {"space b", "workbench.action.toggleSidebarVisibility", "editorFocus && normalMode || sidebarFocus || searchFocus"},
        {"space e", "workbench.view.explorer", "editorFocus && normalMode || searchFocus"},
        {"space e", "noni.focus.editor", "sidebarFocus"},
        {"space s", "workbench.action.findInFiles", "editorFocus && normalMode || searchFocus || sidebarFocus"},
        {"space t", "workbench.action.terminal.toggle", "editorFocus && normalMode || terminalFocus || sidebarFocus || searchFocus"},
        {"space p", "editor.action.clipboardPasteAction", "editorFocus && normalMode"},
        {"tab", "noni.focus.toggleSidebar", "editorFocus && normalMode || sidebarFocus"},
        {":", "noni.command.open", "editorFocus && normalMode"},
        {"g t", "workbench.action.nextEditor", "editorFocus && normalMode"},
        {"g shift+t", "workbench.action.previousEditor", "editorFocus && normalMode"},
        {"u", "editor.action.undo", "editorFocus && normalMode"},
        {"ctrl+r", "editor.action.redo", "editorFocus && normalMode"},
        {"ctrl+w v", "workbench.action.splitEditorRight", "editorFocus"},
        {"ctrl+w s", "workbench.action.splitEditorDown", "editorFocus"},
        {"ctrl+w q", "workbench.action.closeActiveEditorGroup", "editorFocus"},
        {"ctrl+w h", "workbench.action.focusLeftGroup", "editorFocus"},
        {"ctrl+w l", "workbench.action.focusRightGroup", "editorFocus"},
        {"ctrl+w k", "workbench.action.focusAboveGroup", "editorFocus"},
        {"ctrl+w j", "workbench.action.focusBelowGroup", "editorFocus"},
        {"ctrl+w <", "workbench.action.decreaseViewWidth", "editorFocus"},
        {"ctrl+w >", "workbench.action.increaseViewWidth", "editorFocus"},
        {"ctrl+w -", "workbench.action.decreaseViewHeight", "editorFocus"},
        {"ctrl+w +", "workbench.action.increaseViewHeight", "editorFocus"},
    };
    return cfg;
}

AppConfig AppConfig::load(const std::string &path)
{
    AppConfig cfg = defaults();

    std::ifstream file(path);
    if (!file)
    {
        Logger::warning(std::format("Config not found ({}), using defaults", path));
        return cfg;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string text = ss.str();

    try
    {
        const MiniJson::Value root = MiniJson::parse(text);
        cfg.app_name = root.get_string("appName", cfg.app_name);
        cfg.sidebar_width = root.get_int("sidebarWidth", cfg.sidebar_width);
        cfg.line_number_width = root.get_int("lineNumberWidth", cfg.line_number_width);
        cfg.esc_delay_ms = root.get_int("escDelayMs", cfg.esc_delay_ms);
        if (const MiniJson::Value *v = root.get("syntaxAutoInstall"); v && v->is_bool())
            cfg.syntax_auto_install = v->as_bool(cfg.syntax_auto_install);

        if (const MiniJson::Value *arr = root.get("keybindings"); arr && arr->is_array())
        {
            cfg.keybindings.clear();
            for (const auto &item : arr->as_array())
            {
                Keybinding kb;
                kb.key = item.get_string("key", "");
                kb.command = item.get_string("command", "");
                kb.when = item.get_string("when", "");
                if (!kb.key.empty() && !kb.command.empty())
                    cfg.keybindings.push_back(std::move(kb));
            }
        }

        if (const MiniJson::Value *lsp = root.get("lsp"); lsp && lsp->is_object())
        {
            if (const MiniJson::Value *servers = lsp->get("servers"); servers && servers->is_array())
            {
                cfg.lsp_servers.clear();
                for (const auto &item : servers->as_array())
                {
                    if (!item.is_object())
                        continue;
                    LspServerConfigFile sc;
                    sc.language = item.get_string("language", "");
                    if (const MiniJson::Value *cmd = item.get("command"); cmd && cmd->is_array())
                    {
                        for (const auto &c : cmd->as_array())
                        {
                            if (c.is_string())
                                sc.command.push_back(c.as_string());
                        }
                    }
                    if (const MiniJson::Value *markers = item.get("rootMarkers"); markers && markers->is_array())
                    {
                        for (const auto &m : markers->as_array())
                        {
                            if (m.is_string())
                                sc.root_markers.push_back(m.as_string());
                        }
                    }
                    if (!sc.language.empty() && !sc.command.empty())
                        cfg.lsp_servers.push_back(std::move(sc));
                }
            }
        }

        if (const MiniJson::Value *term = root.get("terminal"); term && term->is_object())
        {
            cfg.terminal.shell = term->get_string("shell", cfg.terminal.shell);
            cfg.terminal.height = term->get_int("height", cfg.terminal.height);
            cfg.terminal.scrollback = term->get_int("scrollback", cfg.terminal.scrollback);
        }

        Logger::info(std::format(
            "Config loaded: {} ({} keybindings, {} lsp servers)",
            path,
            cfg.keybindings.size(),
            cfg.lsp_servers.size()));
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("Config parse failed: {}", e.what()));
    }

    return cfg;
}
