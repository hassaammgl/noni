#pragma once

#include <editor/buffer_manager.hpp>
#include <ui/UIComponent.hpp>
#include <utils/fuzzy.hpp>

#include <string>
#include <string_view>
#include <vector>

// Fuzzy picker over open editor tabs (not a second index framework).
class BufferPicker : public UIComponent
{
private:
    bool active_ = false;
    std::string query_;
    int selected_ = 0;
    int scroll_y_ = 0;
    std::vector<FuzzyMatch> matches_;
    BufferManager *buffers_ = nullptr;

    void refilter();
    void ensure_selection_visible();

public:
    void bind(BufferManager *buffers) { buffers_ = buffers; }

    void draw() override;

    void open();
    void close();
    bool is_active() const { return active_; }

    void handle_input(int key);
    void insert_utf8(std::string_view utf8);
    bool take_selection(int &out_tab_index);

    const std::string &get_query() const { return query_; }
};
