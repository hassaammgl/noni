#pragma once

#include <ui/UIComponent.hpp>

class LineNumber : public UIComponent
{
private:
    int scroll_y = 0;
    int active_line = 0;
    int total_lines = 0;

public:
    void draw() override;

    void sync(int scroll_y, int active_line, int total_lines);
};
