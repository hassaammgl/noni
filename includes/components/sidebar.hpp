#pragma once

#include "ui/UIComponent.hpp"
#include "utils/fs.hpp"
#include "utils/logger.hpp"
#include <string>

class Sidebar : public UIComponent
{
private:
    Logger l;
    FS fs;
    std::string title = "File manager";

public:
    Sidebar() = default;
    ~Sidebar() = default;
    void draw() override;
};