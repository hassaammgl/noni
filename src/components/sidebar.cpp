#include <components/sidebar.hpp>
#include <ui/icons.hpp>
#include <ui/theme.hpp>

void Sidebar::draw()
{
    if (!window)
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Sidebar));
    std::wstring title =
        get_project_path().filename().wstring();
    wattron(window, COLOR_PAIR(Theme::SidebarTitle));
    mvwaddwstr(window, 0, 0, Icons::folder_opened);
    waddwstr(window, title.c_str());
    wattroff(window, COLOR_PAIR(Theme::SidebarTitle));

    const auto &file_entries = ds.get_entries();

    if (file_entries.empty())
    {
        mvwaddwstr(window, 1, 0, L"Empty Folder...");
        return;
    }

    int row = 1;
    int index = 0;

    for (const auto &entry : file_entries)
    {
        if (row >= getmaxy(window))
            break;

        std::wstring name =
            entry.entry_path.filename().wstring();

        const bool selected = index == selected_index;
        const short pair = selected
                               ? Theme::SidebarSelected
                           : entry.is_dir ? Theme::SidebarDir
                                          : Theme::SidebarFile;

        wattron(window, COLOR_PAIR(pair));
        if (selected)
            mvwhline(window, row, 0, ' ', getmaxx(window));

        if (entry.is_dir)
        {
            mvwaddwstr(window, row, 0, L"> ");
            waddwstr(window, Icons::folder);
            waddwstr(window, L" ");
            waddwstr(window, name.c_str());
        }
        else if (entry.is_file)
        {
            mvwaddwstr(window, row, 0, L"  ");
            waddwstr(window, Icons::file);
            waddwstr(window, L" ");
            waddwstr(window, name.c_str());
        }
        wattroff(window, COLOR_PAIR(pair));

        ++row;
        ++index;
    }
}

fs::path Sidebar::get_project_path()
{
    return this->project_path;
}

void Sidebar::set_project_path(const fs::path &project_path)
{
    this->project_path = project_path;
    ds.set_project_path(project_path);
    ds.scan_dirs();
    selected_index = 0;
}

void Sidebar::handle_input(int key)
{
    const auto &entries = ds.get_entries();
    if (entries.empty())
    {
        selected_index = 0;
        return;
    }

    if (selected_index >= static_cast<int>(entries.size()))
        selected_index = static_cast<int>(entries.size()) - 1;

    switch (key)
    {
    case KEY_UP:
        if (selected_index > 0)
            selected_index--;
        break;
    case KEY_DOWN:
        if (selected_index + 1 < static_cast<int>(entries.size()))
            selected_index++;
        break;

    default:
        break;
    }
}

fs::path Sidebar::get_selected_path() const
{
    const auto &entries = ds.get_entries();
    if (entries.empty() || selected_index < 0 || selected_index >= static_cast<int>(entries.size()))
    {
        return {};
    }
    return entries[selected_index].entry_path;
}
