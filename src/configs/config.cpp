#include <configs/config.hpp>
#include <utils/logger.hpp>

#include <cstdlib>
#include <format>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    fs::path exe_directory()
    {
        char buf[4096];
        const ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (n <= 0)
            return {};
        buf[n] = '\0';
        return fs::path(buf).parent_path();
    }

    std::vector<fs::path> config_candidates()
    {
        std::vector<fs::path> out;
        auto push = [&](fs::path p) {
            if (p.empty())
                return;
            std::error_code ec;
            fs::path norm = fs::weakly_canonical(p, ec);
            if (ec)
                norm = p;
            for (const auto &e : out)
            {
                if (e == norm)
                    return;
            }
            out.push_back(std::move(norm));
        };

        std::error_code ec;
        const fs::path cwd = fs::current_path(ec);
        if (!ec)
            push(cwd / "config.json");

        const fs::path exe = exe_directory();
        if (!exe.empty())
            push(exe / "config.json");

        if (const char *home = std::getenv("HOME"); home && home[0])
            push(fs::path(home) / ".config" / "noni" / "config.json");

        return out;
    }

    void apply_json(AppConfig &cfg, const MiniJson::Value &root)
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

        if (const MiniJson::Value *exts = root.get("extensions"); exts && exts->is_object())
        {
            cfg.extensions.clear();
            for (const auto &[k, v] : exts->as_object())
                cfg.extensions.emplace(k, v);
        }
    }
}

AppConfig AppConfig::defaults()
{
    AppConfig cfg;
    cfg.keybindings = {};
    return cfg;
}

AppConfig AppConfig::load_file(const std::string &path)
{
    AppConfig cfg = defaults();
    cfg.loaded_from = path;

    std::ifstream file(path);
    if (!file)
    {
        cfg.loaded_from.clear();
        cfg.load_message = std::format("Config not found ({})", path);
        Logger::warning(std::format("{}, using defaults", cfg.load_message));
        return cfg;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    try
    {
        apply_json(cfg, MiniJson::parse(ss.str()));
        Logger::info(std::format(
            "Config loaded: {} ({} keybindings, {} lsp servers, {} extension configs)",
            path,
            cfg.keybindings.size(),
            cfg.lsp_servers.size(),
            cfg.extensions.size()));
    }
    catch (const std::exception &e)
    {
        AppConfig failed = defaults();
        failed.loaded_from = path;
        failed.load_message = std::format("Config parse failed ({}): {}", path, e.what());
        Logger::error(failed.load_message);
        return failed;
    }

    return cfg;
}

AppConfig AppConfig::load()
{
    const auto candidates = config_candidates();
    for (const auto &p : candidates)
    {
        std::error_code ec;
        if (!fs::is_regular_file(p, ec) || ec)
            continue;
        return load_file(p.string());
    }

    AppConfig cfg = defaults();
    cfg.load_message =
        "config.json not found (looked cwd, exe dir, ~/.config/noni/); keybindings empty";
    Logger::warning(cfg.load_message);
    for (const auto &p : candidates)
        Logger::warning(std::format("  tried: {}", p.string()));
    return cfg;
}
