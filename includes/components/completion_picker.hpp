#pragma once

#include <lsp/completion.hpp>
#include <ui/UIComponent.hpp>

#include <string>

// Lightweight completion popup (reuses Popup theme; not a second picker framework).
class CompletionPicker : public UIComponent
{
private:
    bool active_ = false;
    CompletionList list_;
    int selected_ = 0;
    int scroll_y_ = 0;

    void ensure_selection_visible();

public:
    void draw() override;

    void open(CompletionList list);
    void close();
    bool is_active() const { return active_; }

    void handle_input(int key);
    bool take_selection(CompletionItem &out);
    const CompletionList &list() const { return list_; }
};
