#pragma once

#include <ui/UIComponent.hpp>
#include <string>

class MessagesPanel : public UIComponent
{
private:
    bool active = false;
    int scroll_y = 0;
    std::string title_ = "Messages";

public:
    void draw() override;

    void open(std::string title = "Messages");
    void close();
    bool is_active() const;

    void handle_input(int key);
};
