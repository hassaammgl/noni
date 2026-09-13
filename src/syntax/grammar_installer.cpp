#include <syntax/grammar_installer.hpp>
#include <utils/async.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>

#include <cstdlib>
#include <format>
#include <mutex>
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

    struct InstallSpec
    {
        const char *so_name;
        const char *git_url;
        const char *src_rel; // relative to clone root, directory containing parser.c
    };

    const InstallSpec *install_spec(const std::string &so_name)
    {
        static const InstallSpec specs[] = {
            {"c.so", "https://github.com/tree-sitter/tree-sitter-c", "src"},
            {"cpp.so", "https://github.com/tree-sitter/tree-sitter-cpp", "src"},
            {"python.so", "https://github.com/tree-sitter/tree-sitter-python", "src"},
            {"javascript.so", "https://github.com/tree-sitter/tree-sitter-javascript", "src"},
            {"rust.so", "https://github.com/tree-sitter/tree-sitter-rust", "src"},
            {"bash.so", "https://github.com/tree-sitter/tree-sitter-bash", "src"},
            {"json.so", "https://github.com/tree-sitter/tree-sitter-json", "src"},
            {"lua.so", "https://github.com/tree-sitter-grammars/tree-sitter-lua", "src"},
            {"markdown.so",
             "https://github.com/tree-sitter-grammars/tree-sitter-markdown",
             "tree-sitter-markdown/src"},
            {"go.so", "https://github.com/tree-sitter/tree-sitter-go", "src"},
            {"java.so", "https://github.com/tree-sitter/tree-sitter-java", "src"},
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

    int run_cmd(const std::string &cmd)
    {
        Logger::debug(std::format("grammar-install: {}", cmd));
        return std::system(cmd.c_str());
    }

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

        fs::create_directories(out_so.parent_path());
        const fs::path tmp = out_so.string() + ".tmp";

        std::string cmd = cxx ? "g++" : "cc";
        cmd += " -shared -fPIC -O2";
        cmd += " -I" + src_dir.string();
        for (const auto &s : sources)
        {
            cmd += " \"";
            cmd += s.string();
            cmd += "\"";
        }
        cmd += " -o \"";
        cmd += tmp.string();
        cmd += "\"";

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
        const fs::path src_root = root / "src";
        // Use full repo folder name from URL
        std::string repo_name = spec.git_url;
        if (const auto pos = repo_name.find_last_of('/'); pos != std::string::npos)
            repo_name = repo_name.substr(pos + 1);
        const fs::path repo_dir = src_root / repo_name;
        const fs::path out_so = root / spec.so_name;

        fs::create_directories(src_root);

        if (!fs::exists(repo_dir / "src") && !fs::exists(repo_dir / spec.src_rel))
        {
            // Fresh shallow clone
            const std::string cmd = std::format(
                "git clone --depth 1 --single-branch \"{}\" \"{}\"",
                spec.git_url,
                repo_dir.string());
            if (run_cmd(cmd) != 0)
            {
                Logger::error(std::format("grammar-install: clone failed for {}", spec.so_name));
                return false;
            }
        }

        const fs::path src_dir = repo_dir / spec.src_rel;
        if (!compile_parser(src_dir, out_so))
        {
            Logger::error(std::format("grammar-install: compile failed for {}", spec.so_name));
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

fs::path GrammarInstaller::user_dir()
{
    const char *home = std::getenv("HOME");
    if (!home || !*home)
        return fs::path("/tmp/noni-tree-sitter");
    return fs::path(home) / ".local" / "share" / "noni" / "tree-sitter";
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

    const fs::path user = so_path(so_name);
    if (fs::exists(user))
        return user;

    return {};
}

std::uint64_t GrammarInstaller::generation()
{
    return g_generation.load(std::memory_order_relaxed);
}

GrammarInstaller::Status GrammarInstaller::status(const std::string &so_name)
{
    std::lock_guard lock(g_mu);
    if (const auto it = g_status.find(so_name); it != g_status.end())
        return it->second;
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
        if (g_queued.count(so) || g_status[so] == Status::Installing ||
            g_status[so] == Status::Failed)
            return;
        g_queued.insert(so);
        g_status[so] = Status::Installing;
    }

    Messages::info(std::format("Installing tree-sitter grammar: {}…", so));
    Logger::info(std::format("grammar-install: queued {}", so));

    // Cpp also benefits from C if cpp fails later; queue cpp only here.

    const std::string so_name = so;
    InstallSpec spec_copy = *spec;

    Background::instance().post([so_name, spec_copy]() {
        const bool ok = install_one(spec_copy);
        {
            std::lock_guard lock(g_mu);
            g_queued.erase(so_name);
            g_status[so_name] = ok ? Status::Ready : Status::Failed;
        }
        if (ok)
        {
            g_generation.fetch_add(1, std::memory_order_relaxed);
            Messages::info(std::format("Tree-sitter grammar ready: {}", so_name));
        }
        else
        {
            Messages::warning(std::format(
                "Tree-sitter install failed: {} (using lexer)", so_name));
        }
    });
}
