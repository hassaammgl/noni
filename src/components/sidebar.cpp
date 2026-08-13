#include "components/sidebar.hpp"
#include <vector>

void Sidebar::draw()
{

    if (!window)
        return;

    werase(window);
    wbkgd(window, COLOR_PAIR(3));
    mvwprintw(
        window,
        0,
        1, "%s",
        this->title.c_str());
    std::vector<fs::path> dirs = this->fs.listDirectory(this->fs.currentPath());
    for (size_t i = 0; i < dirs.size(); i++)
    {
        mvwprintw(
            window,
            2 + i,
            1, "%s",
            dirs.at(i).filename().string().c_str());
    }
}