#include "components/header.hpp"

void Header::draw()
{
    if (!window)
        return;

    werase(window);
    wbkgd(window, COLOR_PAIR(2));
    mvwprintw(
        window,
        0,
        1,
        "NONI EDITOR");
    int branchnameX =
        width - static_cast<int>(this->branchname.length()) - 2;

    if (branchnameX < 0)
        branchnameX = 0;

    mvwprintw(
        window,
        0,
        branchnameX,
        "%s",
        this->branchname.c_str());
}

std::string Header::getProjectGitBranch()
{
    std::string br = this->branchname;
    return br;
}
