#pragma once

#include <ui/UIComponent.hpp>
#include <string>

enum class ConfirmChoice
{
    Pending,
    Yes,
    No,
    Cancel,
};

class ConfirmPrompt : public UIComponent
{
private:
    bool active = false;
    std::string message;

public:
    void draw() override;

    void open(const std::string &message);
    void close();
    bool is_active() const;

    ConfirmChoice handle_input(int key);
};
