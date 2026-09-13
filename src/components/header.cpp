#include <components/header.hpp>
#include <ui/theme.hpp>
#include <ui/icons.hpp>
#include <utils/async.hpp>

void Header::draw()
{
    if (!window)
        return;

    std::string branch;
    {
        std::lock_guard lock(branch_mu);
        branch = branch_name;
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

std::string Header::get_project_git_branch()
{
    std::lock_guard lock(branch_mu);
    return branch_name;
}

void Header::refresh_git(const fs::path &project_root)
{
    {
        std::lock_guard lock(branch_mu);
        branch_name = "…";
    }

    const std::uint64_t token = Background::instance().next_token();
    git_token.store(token, std::memory_order_relaxed);

    Background::instance().post([this, project_root, token]() {
        if (git_token.load(std::memory_order_relaxed) != token)
            return;

        Git local(project_root);
        std::string name = "[no git]";
        if (auto branch = local.current_branch())
            name = *branch;

        if (git_token.load(std::memory_order_relaxed) != token)
            return;

        std::lock_guard lock(branch_mu);
        branch_name = std::move(name);
    });
}

bool Header::poll()
{
    return false;
}
