#pragma once
#include <lsp/lsp_installer.hpp>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
namespace fs = std::filesystem;
namespace lsp_installer_state
{
    extern std::mutex g_mu;
    extern bool g_auto_install;
    extern std::atomic<std::uint64_t> g_generation;
    extern std::unordered_map<std::string, LspInstaller::Status> g_status;
    extern std::unordered_set<std::string> g_queued;
    extern std::unordered_map<std::string, std::chrono::steady_clock::time_point> g_failed_at;
    inline constexpr auto kFailedRetry = std::chrono::seconds(120);
    extern fs::path g_workspace_root;
    extern fs::path g_user_dir_cache;
    extern bool g_user_dir_resolved;
    bool dir_writable(const fs::path &dir);
    fs::path xdg_lsp_dir();
    fs::path workspace_lsp_dir();
    fs::path tmp_lsp_dir();
    void set_status(const std::string &name, LspInstaller::Status st);
    fs::path which_on_path(const std::string &name);
}
