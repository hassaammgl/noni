#pragma once

#include <ui/UIComponent.hpp>

#include <mutex>
#include <string>

// Header displays SCM branch; does not execute Git.
class Header : public UIComponent
{
private:
    std::string branch_name_ = "[no git]";
    mutable std::mutex mu_;

public:
    void draw() override;
    void set_branch(std::string branch);
    std::string branch() const;
};
