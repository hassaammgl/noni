#pragma once

// Shared internals for lsp_installer*.cpp (not part of public API).

#include <lsp/lsp_installer.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace lsp_install_detail
{
    enum class Method
    {
        PipVenv,
        NpmPrefix,
        CurlGithubZip,
        GoInstall,
    };

    struct Spec
    {
        const char *binary;
        Method method;
        const char *package;
        const char *extra;
    };

    const Spec *spec_for(const std::string &binary);
    bool exe_ok(const char *name);
    int run_cmd(const std::string &cmd);
    bool link_bin(const fs::path &target, const fs::path &link);
    fs::path find_named_binary(const fs::path &root, const std::string &name);
    bool install_one(const Spec &spec);

    // All known auto-install recipes (null-terminated by binary==nullptr sentinel via size).
    const Spec *catalog_specs(std::size_t &count);
}
