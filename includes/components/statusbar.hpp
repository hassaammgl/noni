#pragma once

#include <ui/UIComponent.hpp>
#include <string>
#include <utils/cursor.hpp>

class Statusbar : public UIComponent
{
private:
    std::string mode = "NORMAL";
    std::string filename = "[No Name]";
    Cursor cursor = {.line = 1, .column = 1};

public:
    void draw() override;
    void set_mode(const std::string &mode);
    void set_filename(const std::string &filename);
    void set_cursor_position(int line, int column);
};
