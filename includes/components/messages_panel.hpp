#pragma once

#include <ui/UIComponent.hpp>

class MessagesPanel : public UIComponent
{
private:
    bool active = false;
    int scroll_y = 0;

public:
    void draw() override;

    void open();
    void close();
    bool is_active() const;

    void handle_input(int key);
};
