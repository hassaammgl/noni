#pragma once

#include <editor/editor_tab.hpp>

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class BufferManager
{
private:
    std::vector<EditorTab> tabs;
    int active_index = -1;

    int find_by_path(const fs::path &path) const;

public:
    void open_untitled();
    void open_file(const fs::path &path);
    bool close_active(bool force = false);

    void switch_to(int index);
    void next_tab();
    void prev_tab();

    EditorTab &active();
    const EditorTab &active() const;

    int get_active_index() const;
    const std::vector<EditorTab> &get_tabs() const;
    bool has_tabs() const;
    std::size_t size() const;
};
