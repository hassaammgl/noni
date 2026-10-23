#pragma once

#include <ui/UIComponent.hpp>
#include <string>
#include <string_view>

class CommandLine : public UIComponent
{
private:
    std::string input;
    bool active = false;
    char prompt_ = ':'; // ':' ex, '/' forward search, '?' backward search

public:
    void draw() override;

    void open(char prompt = ':');
    void close();
    bool is_active() const;
    char prompt() const;

    void handle_input(int key);
    void insert_utf8(std::string_view utf8);
    void clear_input();

    const std::string &get_input() const;
};
