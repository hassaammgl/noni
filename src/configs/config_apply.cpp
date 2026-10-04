#include <configs/config.hpp>

void config_parse_features(AppConfig &cfg, const MiniJson::Value &root);

void config_apply_json(AppConfig &cfg, const MiniJson::Value &root)
{
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
        if (const MiniJson::Value *ai = lsp->get("autoInstall"); ai && ai->is_bool())
            cfg.lsp_auto_install = ai->as_bool(cfg.lsp_auto_install);
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
                if (const MiniJson::Value *markers = item.get("rootMarkers");
                    markers && markers->is_array())
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

    if (const MiniJson::Value *exts = root.get("extensions"); exts && exts->is_object())
    {
        cfg.extensions.clear();
        for (const auto &[k, v] : exts->as_object())
            cfg.extensions.emplace(k, v);
    }

    config_parse_features(cfg, root);
}
