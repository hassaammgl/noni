#pragma once

#include <ui/UIComponent.hpp>
#include <utils/fs.hpp>
#include <utils/logger.hpp>
#include <sidebar/dirscanner.hpp>
#include <string>
#include <unordered_set>
struct VisibleRow
{
    const ScanedEntry *entry;
    int depth;
};

class Sidebar : public UIComponent
{
private:
    FS fs;
    fs::path project_path;
    DirScanner ds;
    int selected_index = 0;
    std::unordered_set<std::string> expanded;
    void collect_visible(const std::vector<ScanedEntry> &entries, int depth, std::vector<VisibleRow> &out) const;

public:
    void draw() override;
    fs::path get_project_path();
    void set_project_path(const fs::path &project_path);
    void handle_input(int key);
    fs::path get_selected_path() const;
    void toggle_expand(const fs::path &path);
};