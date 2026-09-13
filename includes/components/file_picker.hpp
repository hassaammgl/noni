#pragma once

#include <ui/UIComponent.hpp>
#include <utils/fuzzy.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class FilePicker : public UIComponent
{
private:
    bool active = false;
    std::string query;
    int selected = 0;
    int scroll_y = 0;
    FileIndex index;
    std::vector<FuzzyMatch> matches;
    std::uint64_t seen_version = 0;

    void refilter();
    void ensure_selection_visible();

public:
    void draw() override;

    void open(const fs::path &project_root);
    void close();
    bool is_active() const;

    // Kick off indexing early (non-blocking).
    void warm(const fs::path &project_root);
    void reindex();

    // Apply finished background index / keep UI fresh. Returns true if redraw useful.
    bool poll();

    void handle_input(int key);
    bool take_selection(fs::path &out_path);

    const std::string &get_query() const;
};
