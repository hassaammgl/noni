#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Crash recovery snapshots — NEVER written over source files.
// Layout: <workspace>/.noni/recovery/<id>.txt + <id>.meta.json
struct RecoveryEntry
{
    std::string id;
    fs::path original_path;
    fs::path text_path;
    fs::path meta_path;
};

namespace RecoveryStore
{
    fs::path recovery_dir(const fs::path &workspace_root);
    std::string path_id(const fs::path &path);

    bool write_snapshot(
        const fs::path &workspace_root,
        const fs::path &original_path,
        const std::vector<std::string> &lines,
        std::string *error = nullptr);

    void clear_snapshot(const fs::path &workspace_root, const fs::path &original_path);

    std::vector<RecoveryEntry> list(const fs::path &workspace_root);

    std::optional<std::vector<std::string>> read_lines(
        const RecoveryEntry &entry,
        std::string *error = nullptr);
}
