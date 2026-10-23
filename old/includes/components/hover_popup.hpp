#pragma once

#include <ui/UIComponent.hpp>

#include <string>
#include <vector>

// Scrollable hover overlay (LSP K). Esc closes via UI.
class HoverPopup : public UIComponent
{
    std::vector<std::string> lines_;
    int scroll_ = 0;
    bool active_ = false;

public:
    void draw() override;
    void open(std::string title, std::string body);
    void close();
    bool is_active() const { return active_; }
    void handle_input(int key);
};
