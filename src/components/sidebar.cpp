#include "components/sidebar.hpp"
#include "ui/icons.hpp"

void Sidebar::draw()
{
    if (!window)
        return;

    werase(window);
    wbkgd(window, COLOR_PAIR(3));

    std::wstring title =
        getProjectPath().filename().wstring();
    mvwaddwstr(window, 0, 0, Icons::FolderOpened);
    waddwstr(window, title.c_str());

    const auto &fileEntries = ds.getEntries();

    if (fileEntries.empty())
    {
        mvwaddwstr(window, 1, 0, L"Empty Folder...");
        wrefresh(window);
        return;
    }

    int row = 1;

    for (const auto &entry : fileEntries)
    {
        if (row >= getmaxy(window))
            break;

        std::wstring name =
            entry.entriePath.filename().wstring();

        if (entry.isDir)
        {
            mvwaddwstr(window, row, 0, L"> ");
            waddwstr(window, Icons::Folder);
            waddwstr(window, L" ");
            waddwstr(window, name.c_str());
        }
        else if (entry.isFile)
        {
            mvwaddwstr(window, row, 0, L"  ");
            waddwstr(window, Icons::File);
            waddwstr(window, L" ");
            waddwstr(window, name.c_str());
        }

        ++row;
    }

    wrefresh(window);
}

fs::path Sidebar::getProjectPath()
{
    return this->projectPath;
}

void Sidebar::setProjectPath(const fs::path &projectPath)
{
    this->projectPath = projectPath;
    ds.setProjectPath(projectPath);
    ds.scanDirs();
}