#include <components/sidebar.hpp>
#include <ui/icons.hpp>
#include <ui/theme.hpp>
#include <format>

void Sidebar::draw()
{
    if (!window)
        return;

    std::vector<VisibleRow> visible;
    collect_visible(ds.get_entries(), 0, visible);

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Sidebar));
    std::wstring title =
        get_project_path().filename().wstring();
    wattron(window, COLOR_PAIR(Theme::SidebarTitle));
    mvwaddwstr(window, 0, 0, Icons::folder_opened);
    waddwstr(window, title.c_str());
    wattroff(window, COLOR_PAIR(Theme::SidebarTitle));

    if (visible.empty())
    {
        mvwaddwstr(window, 1, 0, L"Empty Folder...");
        return;
    }

    int current_row = 1;
    int max_y = getmaxy(window);

    for (size_t i = 0; i < visible.size(); ++i)
    {
        if (current_row >= max_y)
            break;

        const auto &v_item = visible[i];
        const auto &entry = *v_item.entry;

        std::wstring name = entry.entry_path.filename().wstring();
        const bool selected = (static_cast<int>(i) == selected_index);

        const short pair = (selected
                                ? Theme::SidebarSelected
                                : (entry.is_dir ? Theme::SidebarDir : Theme::SidebarFile));

        wattron(window, COLOR_PAIR(pair));

        if (selected)
            mvwhline(window, current_row, 0, ' ', getmaxx(window));

        std::wstring indent(v_item.depth * 2, L' ');
        mvwaddwstr(window, current_row, 0, indent.c_str());

        if (entry.is_dir)
        {
            bool is_expanded = expanded.count(entry.entry_path.string()) > 0;
            waddwstr(window, is_expanded ? Icons::chevron_down : Icons::chevron_right);
            waddwstr(window, L" ");
            waddwstr(window, is_expanded ? Icons::folder_opened : Icons::folder);
            waddwstr(window, L" ");
            waddwstr(window, name.c_str());
        }
        else if (entry.is_file)
        {
            waddwstr(window, L"  ");
            waddwstr(window, Icons::file);
            waddwstr(window, L" ");
            waddwstr(window, name.c_str());
        }

        wattroff(window, COLOR_PAIR(pair));
        ++current_row;
    }
}

fs::path Sidebar::get_project_path()
{
    return this->project_path;
}

void Sidebar::set_project_path(const fs::path &project_path)
{
    this->project_path = project_path;
    Logger::info(std::format("Sidebar project path set: {}", project_path.string()));
    ds.set_project_path(project_path);
    ds.scan_dirs();
    selected_index = 0;
}

void Sidebar::handle_input(int key)
{
    std::vector<VisibleRow> visible;
    collect_visible(ds.get_entries(), 0, visible);
    if (visible.empty())
    {
        selected_index = 0;
        return;
    }

    if (selected_index >= static_cast<int>(visible.size()))
        selected_index = static_cast<int>(visible.size()) - 1;

    const int previous_index = selected_index;

    switch (key)
    {
    case KEY_UP:
        if (selected_index > 0)
            selected_index--;
        break;
    case KEY_DOWN:
        if (selected_index + 1 < static_cast<int>(visible.size()))
            selected_index++;
        break;
    default:
        Logger::debug(std::format("Sidebar unhandled key={}", key));
        break;
    }

    if (selected_index != previous_index)
    {
        Logger::debug(std::format(
            "Sidebar selection: {} -> {} ({})",
            previous_index,
            selected_index,
            visible[selected_index].entry->entry_path.string()));
    }
}

fs::path Sidebar::get_selected_path() const
{
    std::vector<VisibleRow> visible;
    collect_visible(ds.get_entries(), 0, visible);

    if (visible.empty() || selected_index < 0 || selected_index >= static_cast<int>(visible.size()))
    {
        return {};
    }
    return visible[selected_index].entry->entry_path;
}

void Sidebar::toggle_expand(const fs::path &path)
{
    auto key = path.string();
    if (expanded.count(key))
    {
        expanded.erase(key);
        Logger::info(std::format("Collapsed folder: {}", key));
    }
    else
    {
        expanded.insert(key);
        Logger::info(std::format("Expanded folder: {}", key));
    }
}

void Sidebar::collect_visible(const std::vector<ScanedEntry> &entries, int depth, std::vector<VisibleRow> &out) const
{
    for (const auto &e : entries)
    {
        out.push_back({&e, depth});

        if (e.is_dir && expanded.count(e.entry_path.string()))
            collect_visible(e.inner_entries, depth + 1, out);
    }
}
