#pragma once

#include <ui/UIComponent.hpp>
#include <string>
#include <string_view>

class InputPrompt : public UIComponent
{
private:
    bool active = false;
    std::string prefix;
    std::string input;

public:
    void draw() override;

    void open(const std::string &prefix, const std::string &initial = "");
    void close();
    bool is_active() const;

    // Esc cancels (clears + closes). Typing edits. Enter is handled by UI.
    void handle_input(int key);
    void insert_utf8(std::string_view utf8);
    void clear_input();

    const std::string &get_input() const;
    const std::string &get_prefix() const;
};
