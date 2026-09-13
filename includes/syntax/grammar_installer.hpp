#pragma once

#include <syntax/syntax.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// nvim-style grammar install: clone + compile into ~/.local/share/noni/tree-sitter/
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
