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
        {"ctrl+c", "noni.mode.normal", ""},
        {"ctrl+s", "workbench.action.files.save", "editorFocus"},
        {"ctrl+w", "workbench.action.closeActiveEditor", "editorFocus"},
        {"tab", "noni.focus.toggleSidebar", "editorFocus && normalMode || sidebarFocus"},
        {":", "noni.command.open", "editorFocus && normalMode"},
        {"g t", "workbench.action.nextEditor", "editorFocus && normalMode"},
        {"g shift+t", "workbench.action.previousEditor", "editorFocus && normalMode"},
        {"ctrl+b", "workbench.action.toggleSidebarVisibility", ""},
        {"space e", "workbench.view.explorer", "editorFocus && normalMode || searchFocus"},
        {"space e", "noni.focus.editor", "sidebarFocus"},
        {"space s", "workbench.action.findInFiles", "editorFocus && normalMode || searchFocus || sidebarFocus"},
        {"f2", "workbench.view.explorer", ""},
        {"f3", "workbench.view.search", ""},
        {"ctrl+p", "workbench.action.quickOpen", "editorFocus || sidebarFocus || searchFocus"},
        {"space f", "noni.search.files", "editorFocus && normalMode"},
        {"space t", "workbench.action.terminal.toggle", "editorFocus && normalMode || terminalFocus || sidebarFocus || searchFocus"},
        {"ctrl+`", "workbench.action.terminal.toggle", ""},
        {"f4", "workbench.action.terminal.toggle", ""},
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

        Logger::info(std::format(
            "Config loaded: {} ({} keybindings)",
            path,
            cfg.keybindings.size()));
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("Config parse failed: {}", e.what()));
    }

    return cfg;
}
