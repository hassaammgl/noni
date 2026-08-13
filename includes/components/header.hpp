#pragma once

#include "ui/UIComponent.hpp"
#include <string>

class Header : public UIComponent
{
private:
    std::string branchname = "TODO: [Branch] later";

public:
    void draw() override;
    std::string getProjectGitBranch();
};