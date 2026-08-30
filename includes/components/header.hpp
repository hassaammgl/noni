#pragma once

#include <ui/UIComponent.hpp>
#include <utils/git.hpp>
#include <string>

class Header : public UIComponent
{
private:
    std::string branch_name = "[no git]";
    Git git;

public:
    void draw() override;
    std::string get_project_git_branch();
    void refresh_git(const fs::path &project_root);
};