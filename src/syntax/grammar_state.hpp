#pragma once
#include <syntax/grammar_installer.hpp>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
namespace fs = std::filesystem;
namespace grammar_install_state
{
    struct InstallSpec
    {
        const char *so_name;
        const char *git_url;
        const char *src_rel;
        const char *revision;
    };
    extern std::mutex g_mu;
    extern bool g_auto_install;
    extern std::atomic<std::uint64_t> g_generation;
    extern std::unordered_map<std::string, GrammarInstaller::Status> g_status;
    extern std::unordered_set<std::string> g_queued;
    extern std::unordered_map<std::string, std::chrono::steady_clock::time_point> g_failed_at;
    inline constexpr auto kFailedRetry = std::chrono::seconds(60);
    extern fs::path g_workspace_root;
    extern fs::path g_user_dir_cache;
    extern bool g_user_dir_resolved;
    bool dir_writable(const fs::path &dir);
    fs::path xdg_grammar_dir();
    fs::path workspace_grammar_dir();
    fs::path tmp_grammar_dir();
    const InstallSpec *install_spec(const std::string &so_name);
    const char *so_for_language(Language lang);
    bool exe_ok(const char *name);
    int run_cmd(const std::string &cmd);
    std::string repo_basename(const std::string &url);
    bool download_tarball(const InstallSpec &spec, const fs::path &cache_folder, const fs::path &repo_dir);
    bool download_git(const InstallSpec &spec, const fs::path &cache_folder, const fs::path &repo_dir);
    bool compile_parser(const fs::path &src_dir, const fs::path &out_so);
    bool install_one(const InstallSpec &spec);
    void set_status(const std::string &so, GrammarInstaller::Status st);
}
