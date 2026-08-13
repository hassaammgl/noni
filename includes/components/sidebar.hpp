#pragma once

#include "ui/UIComponent.hpp"
#include "utils/fs.hpp"
#include <string>

class Sidebar : public UIComponent
{
private:
    std::string title = "File manager";
    FS fs;

public:
    Sidebar() = default;
    ~Sidebar() = default;
    void draw() override;
};