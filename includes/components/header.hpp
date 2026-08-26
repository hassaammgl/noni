#pragma once

#include <ui/UIComponent.hpp>
#include <string>

class Header : public UIComponent
{
private:
    std::string branch_name = "TODO: [Branch] later";

public:
    void draw() override;
    std::string get_project_git_branch();
};