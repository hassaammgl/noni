#pragma once

#include <ui/UIComponent.hpp>
#include <utils/fs.hpp>
#include <utils/logger.hpp>
#include <sidebar/dirscanner.hpp>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

struct VisibleRow
{
    const ScanedEntry *entry = nullptr;
    int depth = 0;
};

enum class SidebarAction
{
    None,
    OpenFile,
    FocusEditor,
    AddFile,
    AddFolder,
    Rename,
    Delete,
    Cut,
    Copy,
    Paste,
};

enum class SidebarClipboardMode
{
    None,
    Cut,
    Copy,
};

class Sidebar : public UIComponent
{
private:
    FS fs;
    fs::path project_path;
    DirScanner ds;
    std::vector<ScanedEntry> cached_entries;
    std::uint64_t seen_version = 0;
    int selected_index = 0;
    int scroll_y = 0;
    bool focused = false;
    fs::path active_file;
    std::unordered_set<std::string> expanded;
    fs::path clipboard_path;
    SidebarClipboardMode clipboard_mode = SidebarClipboardMode::None;

    void sync_cache();
    void collect_visible(const std::vector<ScanedEntry> &entries, int depth, std::vector<VisibleRow> &out) const;
    std::vector<VisibleRow> visible_rows() const;
    void clamp_selection(int visible_count);
    void ensure_visible(int list_height);
    int list_height() const;
    void refresh_tree_keep_expanded();
    bool is_under(const fs::path &parent, const fs::path &child) const;

public:
    void draw() override;

    fs::path get_project_path() const;
    void set_project_path(const fs::path &project_path);
    void refresh();
    bool poll();

    void set_focused(bool focused);
    void set_active_file(const fs::path &path);
    void reveal_path(const fs::path &path);

    SidebarAction handle_input(int key);
    fs::path get_selected_path() const;
    fs::path get_create_directory() const;
    bool is_selected_dir() const;
    void toggle_expand(const fs::path &path);

    bool create_file_here(const std::string &name);
    bool create_folder_here(const std::string &name);
    bool rename_selected(const std::string &new_name);
    bool delete_selected();
    bool delete_path(const fs::path &path);
    void select_path(const fs::path &path);

    bool cut_selected();
    bool copy_selected();
    bool paste_here(fs::path *moved_from = nullptr, fs::path *moved_to = nullptr);
    void clear_clipboard();
    bool has_clipboard() const;
    SidebarClipboardMode get_clipboard_mode() const;
    fs::path get_clipboard_path() const;
};
