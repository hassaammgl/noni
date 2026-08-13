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
    void setMode(const std::string &mode);
    void setFilename(const std::string &filename);
    void setCursorPosition(int line, int column);
};
