#pragma once

#include <ui/UIComponent.hpp>
#include <string>

class CommandLine : public UIComponent
{
private:
    std::string input;
    bool active = false;

public:
    void draw() override;

    void open();
    void close();
    bool is_active() const;

    void handle_input(int key);
    void clear_input();

    const std::string &get_input() const;
};
