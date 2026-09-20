#include <syntax/grammar_installer.hpp>
#include <utils/async.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>

#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    std::mutex g_mu;
    bool g_auto_install = true;
    std::atomic<std::uint64_t> g_generation{1};
    std::unordered_map<std::string, GrammarInstaller::Status> g_status;
    std::unordered_set<std::string> g_queued;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> g_failed_at;
    constexpr auto kFailedRetry = std::chrono::seconds(60);
    fs::path g_workspace_root;
    fs::path g_user_dir_cache;
    bool g_user_dir_resolved = false;

    bool dir_writable(const fs::path &dir)
    {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec)
            return false;
        const fs::path probe = dir / ".write_test";
        std::FILE *f = std::fopen(probe.c_str(), "w");
        if (!f)
            return false;
        std::fclose(f);
        fs::remove(probe, ec);
        return true;
    }

    fs::path xdg_grammar_dir()
    {
        if (const char *home = std::getenv("HOME"); home && *home)
            return fs::path(home) / ".local" / "share" / "noni" / "tree-sitter";
        return fs::path("/tmp/noni-tree-sitter");
    }

    fs::path workspace_grammar_dir()
    {
        fs::path root;
        {
            std::lock_guard lock(g_mu);
            root = g_workspace_root;
        }
        if (root.empty())
            return {};
        return root / ".noni" / "tree-sitter";
    }

    fs::path tmp_grammar_dir()
    {
        return fs::path("/tmp") /
               std::format("noni-tree-sitter-{}", static_cast<unsigned>(::getuid()));
    }

    // Mirrors classic nvim-treesitter install_info (url + location + branch).
    struct InstallSpec
    {
        const char *so_name;
        const char *git_url;   // https://github.com/.../tree-sitter-foo  (no .git)
        const char *src_rel;   // directory containing parser.c relative to repo root
        const char *revision;  // preferred branch/tag (nvim: branch / revision)
    };

    const InstallSpec *install_spec(const std::string &so_name)
    {
        static const InstallSpec specs[] = {
            {"c.so", "https://github.com/tree-sitter/tree-sitter-c", "src", "master"},
            {"cpp.so", "https://github.com/tree-sitter/tree-sitter-cpp", "src", "master"},
            {"python.so", "https://github.com/tree-sitter/tree-sitter-python", "src", "master"},
            {"javascript.so", "https://github.com/tree-sitter/tree-sitter-javascript", "src", "master"},
            {"rust.so", "https://github.com/tree-sitter/tree-sitter-rust", "src", "master"},
            {"bash.so", "https://github.com/tree-sitter/tree-sitter-bash", "src", "master"},
            {"json.so", "https://github.com/tree-sitter/tree-sitter-json", "src", "master"},
            {"lua.so", "https://github.com/tree-sitter-grammars/tree-sitter-lua", "src", "main"},
            {"markdown.so",
             "https://github.com/tree-sitter-grammars/tree-sitter-markdown",
             "tree-sitter-markdown/src",
             "split_parser"},
            {"go.so", "https://github.com/tree-sitter/tree-sitter-go", "src", "master"},
            {"java.so", "https://github.com/tree-sitter/tree-sitter-java", "src", "master"},
        };
        for (const auto &s : specs)
        {
            if (so_name == s.so_name)
                return &s;
        }
        return nullptr;
    }

    const char *so_for_language(Language lang)
    {
        switch (lang)
        {
        case Language::C:
            return "c.so";
        case Language::Cpp:
            return "cpp.so";
        case Language::Python:
            return "python.so";
        case Language::JavaScript:
        case Language::TypeScript:
            return "javascript.so";
        case Language::Rust:
            return "rust.so";
        case Language::Shell:
            return "bash.so";
        case Language::JSON:
            return "json.so";
        case Language::Lua:
            return "lua.so";
        case Language::Markdown:
            return "markdown.so";
        case Language::Go:
            return "go.so";
        case Language::Java:
            return "java.so";
        default:
            return nullptr;
        }
    }

    bool exe_ok(const char *name)
    {
        // PATH lookup without spawning a shell login profile.
        const char *path = std::getenv("PATH");
        if (!path || !*path)
            return false;
        std::string paths = path;
        std::size_t start = 0;
        while (start <= paths.size())
        {
            const std::size_t end = paths.find(':', start);
            const std::string dir =
                paths.substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (!dir.empty())
            {
                const fs::path cand = fs::path(dir) / name;
                std::error_code ec;
                if (fs::is_regular_file(cand, ec) && !ec)
                {
                    const auto perms = fs::status(cand, ec).permissions();
                    if (!ec && (perms & fs::perms::owner_exec) != fs::perms::none)
                        return true;
                    // Also accept if any execute bit is set.
                    if (!ec && ((perms & fs::perms::group_exec) != fs::perms::none ||
                                (perms & fs::perms::others_exec) != fs::perms::none))
                        return true;
                }
            }
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        // Fallback: let the shell resolve (handles wrappers / different permission models).
        return std::system(std::format("command -v {} >/dev/null 2>&1", name).c_str()) == 0;
    }

    int run_cmd(const std::string &cmd)
    {
        // Keep install chatter out of the TTY (same class of bug as LSP stderr).
        (void)mkdir("logs", 0755);
        const std::string wrapped = cmd + " >>logs/grammar-install.log 2>&1";
        Logger::debug(std::format("grammar-install: {}", cmd));
        return std::system(wrapped.c_str());
    }

    std::string repo_basename(const std::string &url)
    {
        std::string name = url;
        while (!name.empty() && name.back() == '/')
            name.pop_back();
        if (name.size() > 4 && name.substr(name.size() - 4) == ".git")
            name = name.substr(0, name.size() - 4);
        const auto pos = name.find_last_of('/');
        if (pos != std::string::npos)
            name = name.substr(pos + 1);
        return name;
    }

    // nvim-treesitter: prefer curl+tar of GitHub/GitLab archive; git is fallback.
    bool download_tarball(const InstallSpec &spec, const fs::path &cache_folder, const fs::path &repo_dir)
    {
        if (!exe_ok("curl") || !exe_ok("tar"))
            return false;

        const std::string project = repo_basename(spec.git_url);
        std::string url = spec.git_url;
        if (url.size() > 4 && url.substr(url.size() - 4) == ".git")
            url = url.substr(0, url.size() - 4);

        const bool github = url.find("github.com") != std::string::npos;
        const bool gitlab = url.find("gitlab.com") != std::string::npos;
        if (!github && !gitlab)
            return false;

        // Try preferred revision, then common defaults (repos migrated master→main).
        const std::array<const char *, 3> revisions = {
            spec.revision,
            "master",
            "main",
        };

        const fs::path tar_path = cache_folder / (project + ".tar.gz");
        const fs::path tmp_dir = cache_folder / (project + "-tmp");

        for (const char *rev : revisions)
        {
            if (!rev || !*rev)
                continue;

            std::error_code ec;
            fs::remove_all(tmp_dir, ec);
            fs::remove(tar_path, ec);
            fs::remove_all(repo_dir, ec);
            fs::create_directories(tmp_dir, ec);

            std::string archive_url;
            if (github)
                archive_url = std::format("{}/archive/{}.tar.gz", url, rev);
            else
                archive_url = std::format(
                    "{}/-/archive/{}/{}-{}.tar.gz",
                    url,
                    rev,
                    project,
                    rev);

            const std::string curl_cmd = std::format(
                "curl --fail --location --silent --show-error --max-time 120 "
                "-L \"{}\" --output \"{}\"",
                archive_url,
                tar_path.string());
            if (run_cmd(curl_cmd) != 0 || !fs::exists(tar_path))
                continue;

            const std::string tar_cmd = std::format(
                "tar -xzf \"{}\" -C \"{}\"",
                tar_path.string(),
                tmp_dir.string());
            if (run_cmd(tar_cmd) != 0)
                continue;

            // Extracted dir is typically "<repo>-<rev>" (tags like v0.1 → strip leading v like nvim).
            std::string folder_rev = rev;
            if (github && folder_rev.size() >= 2 && folder_rev[0] == 'v' &&
                std::isdigit(static_cast<unsigned char>(folder_rev[1])))
                folder_rev = folder_rev.substr(1);

            const fs::path extracted = tmp_dir / (project + "-" + folder_rev);
            fs::path chosen = extracted;
            if (!fs::exists(chosen))
            {
                // Pick the single top-level directory tar produced.
                chosen.clear();
                for (const auto &entry : fs::directory_iterator(tmp_dir, ec))
                {
                    if (entry.is_directory(ec))
                    {
                        chosen = entry.path();
                        break;
                    }
                }
            }

            if (chosen.empty() || !fs::exists(chosen))
                continue;

            fs::rename(chosen, repo_dir, ec);
            if (ec)
            {
                fs::remove_all(repo_dir, ec);
                fs::rename(chosen, repo_dir, ec);
            }

            fs::remove(tar_path, ec);
            fs::remove_all(tmp_dir, ec);

            if (fs::exists(repo_dir / spec.src_rel / "parser.c") ||
                fs::exists(repo_dir / "src" / "parser.c"))
            {
                Logger::info(std::format(
                    "grammar-install: downloaded {} @ {} (curl+tar)",
                    project,
                    rev));
                return true;
            }
        }

        std::error_code ec;
        fs::remove(tar_path, ec);
        fs::remove_all(tmp_dir, ec);
        fs::remove_all(repo_dir, ec);
        return false;
    }

    bool download_git(const InstallSpec &spec, const fs::path &cache_folder, const fs::path &repo_dir)
    {
        if (!exe_ok("git"))
            return false;

        // Avoid corrupting an active git session (same guard as nvim-treesitter).
        static const char *git_env[] = {
            "GIT_DIR",
            "GIT_INDEX_FILE",
            "GIT_WORK_TREE",
            "GIT_PREFIX",
            "GIT_OBJECT_DIRECTORY",
            nullptr,
        };
        for (const char **e = git_env; *e; ++e)
        {
            if (std::getenv(*e))
            {
                Logger::warning(
                    "grammar-install: skipping git clone inside active git session env");
                return false;
            }
        }

        std::error_code ec;
        fs::remove_all(repo_dir, ec);
        fs::create_directories(cache_folder, ec);

        // nvim classic: git clone --filter=blob:none then checkout revision.
        const std::string clone_cmd = std::format(
            "git -C \"{}\" clone --filter=blob:none \"{}\" \"{}\"",
            cache_folder.string(),
            spec.git_url,
            repo_dir.filename().string());
        if (run_cmd(clone_cmd) != 0 || !fs::exists(repo_dir))
        {
            // Fallback shallow clone (older git without filter).
            const std::string shallow = std::format(
                "git clone --depth 1 --single-branch --branch \"{}\" \"{}\" \"{}\"",
                spec.revision && *spec.revision ? spec.revision : "master",
                spec.git_url,
                repo_dir.string());
            if (run_cmd(shallow) != 0)
                return false;
            return fs::exists(repo_dir);
        }

        if (spec.revision && *spec.revision)
        {
            const std::string co = std::format(
                "git -C \"{}\" checkout \"{}\"",
                repo_dir.string(),
                spec.revision);
            (void)run_cmd(co); // best-effort; tip of default branch still usable
        }

        Logger::info(std::format(
            "grammar-install: cloned {} (git)",
            repo_basename(spec.git_url)));
        return true;
    }

    // Compile flags aligned with nvim-treesitter shell_command_selectors (unix path).
    bool compile_parser(const fs::path &src_dir, const fs::path &out_so)
    {
        const fs::path parser = src_dir / "parser.c";
        if (!fs::exists(parser))
        {
            Logger::error(std::format("grammar-install: missing {}", parser.string()));
            return false;
        }

        std::vector<fs::path> sources{parser};
        bool cxx = false;
        if (fs::exists(src_dir / "scanner.c"))
            sources.push_back(src_dir / "scanner.c");
        if (fs::exists(src_dir / "scanner.cc"))
        {
            sources.push_back(src_dir / "scanner.cc");
            cxx = true;
        }
        if (fs::exists(src_dir / "scanner.cpp"))
        {
            sources.push_back(src_dir / "scanner.cpp");
            cxx = true;
        }

        const char *cc = nullptr;
        if (cxx)
        {
            if (exe_ok("c++"))
                cc = "c++";
            else if (exe_ok("g++"))
                cc = "g++";
            else if (exe_ok("clang++"))
                cc = "clang++";
        }
        else
        {
            if (exe_ok("cc"))
                cc = "cc";
            else if (exe_ok("gcc"))
                cc = "gcc";
            else if (exe_ok("clang"))
                cc = "clang";
        }
        if (!cc)
        {
            Logger::error("grammar-install: no C/C++ compiler found (cc/gcc/clang)");
            return false;
        }

        fs::create_directories(out_so.parent_path());
        const fs::path tmp = out_so.string() + ".tmp";

        // nvim: -o parser.so -I./src files -Os -shared -fPIC [-lstdc++]
        // Use c11 for C-only grammars; C++ scanners need a C++ standard.
        std::string cmd = cc;
        cmd += " -o \"";
        cmd += tmp.string();
        cmd += "\" -I\"";
        cmd += src_dir.string();
        cmd += "\"";
        for (const auto &s : sources)
        {
            cmd += " \"";
            cmd += s.string();
            cmd += "\"";
        }
        cmd += cxx ? " -Os -std=c++14 -shared -fPIC -lstdc++"
                   : " -Os -std=c11 -shared -fPIC";

        if (run_cmd(cmd) != 0)
        {
            fs::remove(tmp);
            return false;
        }

        std::error_code ec;
        fs::rename(tmp, out_so, ec);
        if (ec)
        {
            fs::remove(out_so, ec);
            fs::rename(tmp, out_so, ec);
        }
        return fs::exists(out_so);
    }

    bool install_one(const InstallSpec &spec)
    {
        const fs::path root = GrammarInstaller::user_dir();
        const fs::path cache_folder = root / "src";
        const std::string project = repo_basename(spec.git_url);
        const fs::path repo_dir = cache_folder / project;
        const fs::path out_so = root / spec.so_name;

        Messages::info(std::format(
            "Grammar {} → {}",
            spec.so_name,
            root.string()));

        std::error_code ec;
        fs::create_directories(cache_folder, ec);
        if (ec)
        {
            Logger::error(std::format(
                "grammar-install: cannot write {} ({}) — fix ownership "
                "(e.g. sudo chown -R \"$USER\" ~/.local/share/noni)",
                cache_folder.string(),
                ec.message()));
            Messages::error(std::format(
                "Grammar dir not writable: {}",
                root.string()));
            return false;
        }

        const bool have_src =
            fs::exists(repo_dir / spec.src_rel / "parser.c") ||
            fs::exists(repo_dir / "src" / "parser.c");

        if (!have_src)
        {
            Messages::info(std::format(
                "Downloading {} → {}",
                project,
                repo_dir.string()));
            // nvim order: curl+tar first, then git.
            if (!download_tarball(spec, cache_folder, repo_dir) &&
                !download_git(spec, cache_folder, repo_dir))
            {
                Logger::error(std::format(
                    "grammar-install: download failed for {} (need curl+tar or git)",
                    spec.so_name));
                Messages::warning(std::format(
                    "Download failed: {} (see logs/grammar-install.log)",
                    project));
                return false;
            }
            Messages::info(std::format("Downloaded {}", project));
        }
        else
        {
            Messages::info(std::format("Using cached src {}", repo_dir.string()));
        }

        fs::path src_dir = repo_dir / spec.src_rel;
        if (!fs::exists(src_dir / "parser.c"))
            src_dir = repo_dir / "src";

        Messages::info(std::format("Compiling {} → {}", spec.so_name, out_so.string()));
        if (!compile_parser(src_dir, out_so))
        {
            Logger::error(std::format("grammar-install: compile failed for {}", spec.so_name));
            Messages::warning(std::format(
                "Compile failed: {} (see logs/grammar-install.log)",
                spec.so_name));
            return false;
        }

        Logger::info(std::format("grammar-install: ready {}", out_so.string()));
        return true;
    }

    void set_status(const std::string &so, GrammarInstaller::Status st)
    {
        std::lock_guard lock(g_mu);
        g_status[so] = st;
    }
}

void GrammarInstaller::set_auto_install(bool enabled)
{
    std::lock_guard lock(g_mu);
    g_auto_install = enabled;
}

bool GrammarInstaller::auto_install()
{
    std::lock_guard lock(g_mu);
    return g_auto_install;
}

void GrammarInstaller::set_workspace_root(const fs::path &root)
{
    std::lock_guard lock(g_mu);
    if (g_workspace_root == root)
        return;
    g_workspace_root = root;
    // Re-resolve install dir so we can prefer <ws>/.noni/tree-sitter.
    g_user_dir_resolved = false;
    g_user_dir_cache.clear();
}

fs::path GrammarInstaller::user_dir()
{
    {
        std::lock_guard lock(g_mu);
        if (g_user_dir_resolved)
            return g_user_dir_cache;
    }

    const fs::path preferred = xdg_grammar_dir();
    if (dir_writable(preferred))
    {
        std::lock_guard lock(g_mu);
        g_user_dir_cache = preferred;
        g_user_dir_resolved = true;
        return g_user_dir_cache;
    }

    const fs::path ws = workspace_grammar_dir();
    if (!ws.empty() && dir_writable(ws))
    {
        Logger::warning(std::format(
            "grammar-install: {} not writable — using {} "
            "(fix with: sudo chown -R \"$USER\" ~/.local/share/noni)",
            preferred.string(),
            ws.string()));
        std::lock_guard lock(g_mu);
        g_user_dir_cache = ws;
        g_user_dir_resolved = true;
        return g_user_dir_cache;
    }

    const fs::path tmp = tmp_grammar_dir();
    std::error_code ec;
    fs::create_directories(tmp, ec);
    Logger::warning(std::format(
        "grammar-install: {} not writable — using {} "
        "(fix with: sudo chown -R \"$USER\" ~/.local/share/noni)",
        preferred.string(),
        tmp.string()));
    std::lock_guard lock(g_mu);
    g_user_dir_cache = tmp;
    g_user_dir_resolved = true;
    return g_user_dir_cache;
}

fs::path GrammarInstaller::so_path(const std::string &so_name)
{
    return user_dir() / so_name;
}

fs::path GrammarInstaller::find_grammar(const std::string &so_name)
{
    const fs::path system = fs::path("/usr/lib/tree_sitter") / so_name;
    if (fs::exists(system))
        return system;

    // Preferred XDG path (may be root-owned and read-only — still load .so from it).
    {
        const fs::path preferred = xdg_grammar_dir() / so_name;
        if (fs::exists(preferred))
            return preferred;
    }

    // Project-local cache (used when XDG is broken / unwritable).
    if (const fs::path ws = workspace_grammar_dir(); !ws.empty())
    {
        const fs::path cand = ws / so_name;
        if (fs::exists(cand))
            return cand;
    }

    const fs::path user = so_path(so_name); // writable dir (xdg / .noni / /tmp)
    if (fs::exists(user))
        return user;

    const fs::path tmp = tmp_grammar_dir() / so_name;
    if (fs::exists(tmp))
        return tmp;

    return {};
}

std::uint64_t GrammarInstaller::generation()
{
    return g_generation.load(std::memory_order_relaxed);
}

GrammarInstaller::Status GrammarInstaller::status(const std::string &so_name)
{
    {
        std::lock_guard lock(g_mu);
        if (const auto it = g_status.find(so_name); it != g_status.end())
            return it->second;
    }
    if (!find_grammar(so_name).empty())
        return Status::Ready;
    return Status::Missing;
}

void GrammarInstaller::request(Language lang)
{
    const char *so = so_for_language(lang);
    if (!so)
        return;

    const InstallSpec *spec = install_spec(so);
    if (!spec)
        return;

    if (!find_grammar(so).empty())
    {
        set_status(so, Status::Ready);
        return;
    }

    {
        std::lock_guard lock(g_mu);
        if (!g_auto_install)
            return;
        if (g_queued.count(so) || g_status[so] == Status::Installing)
            return;
        if (g_status[so] == Status::Failed)
        {
            const auto it = g_failed_at.find(so);
            if (it != g_failed_at.end() &&
                std::chrono::steady_clock::now() - it->second < kFailedRetry)
                return;
        }
        g_queued.insert(so);
        g_status[so] = Status::Installing;
        g_failed_at.erase(so);
    }

    const fs::path dest = GrammarInstaller::user_dir();
    Messages::info(std::format(
        "Installing {} → {} …",
        so,
        dest.string()));
    Logger::info(std::format(
        "grammar-install: queued {} → {}",
        so,
        dest.string()));

    const std::string so_name = so;
    InstallSpec spec_copy = *spec;

    Background::instance().post([so_name, spec_copy]() {
        const bool ok = install_one(spec_copy);
        const fs::path out = GrammarInstaller::so_path(so_name);
        {
            std::lock_guard lock(g_mu);
            g_queued.erase(so_name);
            g_status[so_name] = ok ? Status::Ready : Status::Failed;
            if (!ok)
                g_failed_at[so_name] = std::chrono::steady_clock::now();
            else
                g_failed_at.erase(so_name);
        }
        if (ok)
        {
            g_generation.fetch_add(1, std::memory_order_relaxed);
            Messages::info(std::format("Grammar ready: {}", out.string()));
        }
        else
        {
            Messages::warning(std::format(
                "Grammar failed: {} (retry 60s; log: logs/grammar-install.log)",
                so_name));
        }
    });
}
