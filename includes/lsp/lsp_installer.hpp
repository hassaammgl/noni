#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Mason-style LSP bootstrap into ~/.local/share/noni/lsp/ (no sudo).
// Uses pip venv / npm|bun prefix / curl GitHub releases / go install.
class LspInstaller
{
public:
    enum class Status
    {
        Missing,
        Installing,
        Ready,
        Failed,
    };

    static void set_auto_install(bool enabled);
    static bool auto_install();

    static void set_workspace_root(const fs::path &root);

    // Writable root: XDG → <ws>/.noni/lsp → /tmp/noni-lsp-<uid>
    static fs::path user_dir();
    static fs::path bin_dir();

    // Absolute path to binary if on PATH or under bin_dir(); else empty.
    static fs::path resolve(const std::string &command_name);

    // Queue background install when missing and auto_install is on.
    // force=true ignores auto_install (for :lsp UI).
    static void request(const std::string &command_name, bool force = false);

    // Prefetch every configured server binary (call on editor start).
    static void request_all(const std::vector<std::string> &command_names);

    struct CatalogEntry
    {
        std::string binary;
        std::string via;     // pip / npm / curl / go
        std::string package; // package or release id
    };

    static std::vector<CatalogEntry> catalog();

    static Status status(const std::string &command_name);
    static const char *status_label(Status st);
    static std::uint64_t generation();
};
