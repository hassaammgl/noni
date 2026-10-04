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

void config_apply_json(AppConfig &cfg, const MiniJson::Value &root);

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
        config_apply_json(cfg, MiniJson::parse(ss.str()));
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
