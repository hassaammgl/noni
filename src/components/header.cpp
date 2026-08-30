#include <components/header.hpp>
#include <ui/theme.hpp>

void Header::draw()
{
    if (!window)
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Header));
    mvwprintw(
        window,
        0,
        1,
        "NONI EDITOR");
    int branch_name_x =
        width - static_cast<int>(this->branch_name.length()) - 2;

    if (branch_name_x < 0)
        branch_name_x = 0;

    mvwprintw(window, 0, branch_name_x, "%s %s", "branch", branch_name.c_str());
    // or Icons::git_branch if you add wide string support in header
}

std::string Header::get_project_git_branch()
{
    std::string br = this->branch_name;
    return br;
}

void Header::refresh_git(const fs::path &project_root)
{
    git.set_repo_root(project_root);
    if (auto branch = git.current_branch())
        branch_name = *branch;
    else
        branch_name = "[no git]";
}