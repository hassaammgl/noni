#pragma once

#include "ui/UIComponent.hpp"
#include "utils/fs.hpp"
#include "utils/logger.hpp"
#include "sidebar/dirscanner.hpp"
#include <string>

class Sidebar : public UIComponent
{
private:
    Logger l;
    FS fs;
    fs::path projectPath;
    DirScanner ds;

public:
    void draw() override;
    fs::path getProjectPath();
    void setProjectPath(const fs::path &projectPath);
};