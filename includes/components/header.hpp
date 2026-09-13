#pragma once

#include <ui/UIComponent.hpp>
#include <utils/git.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

class Header : public UIComponent
{
private:
    std::string branch_name = "[no git]";
    std::mutex branch_mu;
    std::atomic<std::uint64_t> git_token{0};
    Git git;

public:
    void draw() override;
    std::string get_project_git_branch();
    void refresh_git(const fs::path &project_root);
    bool poll(); // apply async git result; true if changed
};
