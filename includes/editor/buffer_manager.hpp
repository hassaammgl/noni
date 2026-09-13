#pragma once

#include <editor/editor_tab.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

// Owns Buffer lifetimes. Tabs hold Windows that reference those Buffers.
class BufferManager
{
private:
    std::vector<std::unique_ptr<Buffer>> buffers;
    std::vector<EditorTab> tabs;
    int active_index = -1;

    Buffer *create_buffer();
    void destroy_buffer(Buffer *buffer);
    bool buffer_referenced(Buffer *buffer) const;
    int find_tab_by_path(const fs::path &path) const;

public:
    void open_untitled();
    void open_file(const fs::path &path);
    bool close_active(bool force = false);

    void switch_to(int index);
    void next_tab();
    void prev_tab();

    EditorTab &active();
    const EditorTab &active() const;

    Buffer *find_buffer_by_path(const fs::path &path);

    int get_active_index() const;
    const std::vector<EditorTab> &get_tabs() const;
    bool has_tabs() const;
    std::size_t size() const;
};
