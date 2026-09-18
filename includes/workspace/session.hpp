#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Serializable workspace session (paths + view state — not Buffer pointers).
struct SessionTabState
{
    std::string path;
    int cursor_line = 0;
    int cursor_column = 0;
    int scroll_y = 0;
    int scroll_x = 0;
};

struct SessionState
{
    int version = 1;
    std::string workspace_root;
    int active_index = 0;
    bool sidebar_visible = true;
    bool terminal_visible = false;
    std::vector<SessionTabState> tabs;
    std::vector<std::string> recent;
};

// Disk layout: <workspace>/.noni/session.json — no ncurses.
namespace SessionStore
{
    fs::path noni_dir(const fs::path &workspace_root);
    fs::path session_file(const fs::path &workspace_root);

    bool save(const fs::path &workspace_root, const SessionState &state, std::string *error = nullptr);
    std::optional<SessionState> load(const fs::path &workspace_root, std::string *error = nullptr);
}
