#pragma once

#include <ui/UIComponent.hpp>
#include <utils/text_search.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum class SearchField
{
    Query,
    Replace,
    Results,
};

enum class SearchPanelAction
{
    None,
    OpenMatch,
    FocusEditor,
    ReplaceAll,
};

class SearchPanel : public UIComponent
{
private:
    TextSearch engine;
    std::string query;
    std::string replace_text;
    TextSearchOptions opts;
    SearchField field = SearchField::Query;
    int selected = 0;
    int scroll_y = 0;
    bool focused = false;
    std::uint64_t seen_version = 0;
    std::vector<TextMatch> cached;

    void sync_results();
    void ensure_selection_visible(int list_h);
    void run_search();
    int list_top() const;
    int list_height() const;

public:
    void draw() override;

    void set_root(const fs::path &root);
    void set_focused(bool v);
    void open();
    void close();

    bool poll();
    SearchPanelAction handle_input(int key);

    TextMatch selected_match() const;
    std::vector<TextMatch> results() const;
    const std::string &get_query() const;
    const std::string &get_replace() const;
    TextSearchOptions get_options() const;
};
