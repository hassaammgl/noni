#include <components/header.hpp>
#include <ui/theme.hpp>
#include <ui/icons.hpp>

void Header::draw()
{
    if (!window)
        return;

    std::string branch;
    {
        std::lock_guard lock(mu_);
        branch = branch_name_;
    }

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Header));
    wattron(window, COLOR_PAIR(Theme::Header));
    mvwhline(window, 0, 0, ' ', width);

    mvwprintw(window, 0, 1, "NONI");

    const int branch_x = width - static_cast<int>(branch.size()) - 4;
    if (branch_x > 8)
    {
        mvwaddwstr(window, 0, branch_x, Icons::git_branch);
        mvwprintw(window, 0, branch_x + 2, " %s", branch.c_str());
    }
    wattroff(window, COLOR_PAIR(Theme::Header));
}

void Header::set_branch(std::string branch)
{
    std::lock_guard lock(mu_);
    branch_name_ = std::move(branch);
}

std::string Header::branch() const
{
    std::lock_guard lock(mu_);
    return branch_name_;
}
