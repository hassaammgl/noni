#pragma once

#include <ui/UIComponent.hpp>
#include <string>

class Statusbar : public UIComponent
{
private:
    std::string mode = "NORMAL";
    std::string filename = "[No Name]";
    int line_ = 1;          // 1-based line
    int char_ = 1;          // 1-based codepoint index on line
    int display_col_ = 1;   // 1-based display column

public:
    void draw() override;
    void set_mode(const std::string &mode);
    void set_filename(const std::string &filename);
    // line / char / display_col are 1-based user-facing values.
    void set_cursor_position(int line, int char_pos, int display_col);
};
