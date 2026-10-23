#include "grammar_state.hpp"
#include <cstdio>
#include <cstdlib>
#include <format>
#include <unistd.h>
namespace grammar_install_state
{
    std::mutex g_mu;
    bool g_auto_install = true;
    std::atomic<std::uint64_t> g_generation{1};
    std::unordered_map<std::string, GrammarInstaller::Status> g_status;
    std::unordered_set<std::string> g_queued;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> g_failed_at;
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
}
