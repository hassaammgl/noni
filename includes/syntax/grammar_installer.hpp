#pragma once

#include <syntax/syntax.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// nvim-treesitter-style grammar install:
// prefer curl+tar of GitHub archive, fall back to git clone, then compile .so
// into ~/.local/share/noni/tree-sitter/ (or <workspace>/.noni/tree-sitter fallback)
class GrammarInstaller
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

    // Prefer <workspace>/.noni/tree-sitter when XDG data dir is unwritable.
    static void set_workspace_root(const fs::path &root);

    static fs::path user_dir();
    static fs::path so_path(const std::string &so_name); // name.so under user_dir

    // Try system then user paths. Returns empty if not found.
    static fs::path find_grammar(const std::string &so_name);

    // Queue background install if missing and auto_install is on.
    static void request(Language lang);

    // Bumps when any grammar finishes installing successfully.
    static std::uint64_t generation();

    static Status status(const std::string &so_name);
};
