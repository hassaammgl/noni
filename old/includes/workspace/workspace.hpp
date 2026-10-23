#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// Project/workspace root — shared across Windows; not view-local.
class Workspace
{
public:
    // Detect repo/project root from a file or directory hint.
    static fs::path detect_root(const fs::path &hint);

    void open(const fs::path &hint);
    void set_root(fs::path root);
    const fs::path &root() const { return root_; }
    bool has_root() const { return !root_.empty(); }

    std::string display_name() const;

private:
    fs::path root_;
};
