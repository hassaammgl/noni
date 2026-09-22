#include "sidebar_detail.hpp"

using namespace sidebar_detail;

int Sidebar::list_height() const
{
    return std::max(0, height - 1);
}

void Sidebar::sync_cache()
{
    const auto v = ds.version();
    if (v == seen_version)
        return;
    seen_version = v;
    cached_entries = ds.get_entries();
}

bool Sidebar::poll()
{
    const auto before = seen_version;
    sync_cache();
    return seen_version != before;
}

std::vector<VisibleRow> Sidebar::visible_rows() const
{
    std::vector<VisibleRow> visible;
    collect_visible(cached_entries, 0, visible);
    return visible;
}

void Sidebar::clamp_selection(int visible_count)
{
    if (visible_count <= 0)
    {
        selected_index = 0;
        scroll_y = 0;
        return;
    }
    if (selected_index < 0)
        selected_index = 0;
    if (selected_index >= visible_count)
        selected_index = visible_count - 1;
}

void Sidebar::ensure_visible(int lh)
{
    if (lh <= 0)
    {
        scroll_y = 0;
        return;
    }
    if (selected_index < scroll_y)
        scroll_y = selected_index;
    if (selected_index >= scroll_y + lh)
        scroll_y = selected_index - lh + 1;
    if (scroll_y < 0)
        scroll_y = 0;
}

void Sidebar::draw()
{
    if (!window)
        return;

    sync_cache();
    const auto visible = visible_rows();
    clamp_selection(static_cast<int>(visible.size()));
    const int lh = list_height();
    ensure_visible(lh);

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Sidebar));
    wattron(window, COLOR_PAIR(Theme::Sidebar));
    for (int row = 0; row < height; ++row)
        mvwhline(window, row, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::Sidebar));

    // Title bar
    const short title_pair = focused ? Theme::SidebarSelected : Theme::SidebarTitle;
    wattron(window, COLOR_PAIR(title_pair));
    mvwhline(window, 0, 0, ' ', width);
    std::string title = project_path.filename().string();
    if (title.empty() || title == "." || title == "..")
        title = project_path.string();
    if (title.size() > static_cast<std::size_t>(std::max(0, width - 4)))
        title = title.substr(0, static_cast<std::size_t>(std::max(0, width - 5))) + "…";

    mvwaddwstr(window, 0, 1, Icons::folder_opened);
    mvwprintw(window, 0, 3, "%s", title.c_str());
    if (ds.is_scanning())
        mvwprintw(window, 0, std::max(3, width - 4), "…");
    else if (clipboard_mode != SidebarClipboardMode::None)
        mvwprintw(window, 0, std::max(3, width - 5),
                  clipboard_mode == SidebarClipboardMode::Cut ? "CUT" : "CPY");
    else if (focused)
        mvwprintw(window, 0, std::max(3, width - 7), "FOCUS");
    wattroff(window, COLOR_PAIR(title_pair));

    if (visible.empty())
    {
        wattron(window, COLOR_PAIR(Theme::Sidebar));
        mvwprintw(window, 1, 1, ds.is_scanning() ? "Scanning…" : "Empty folder");
        wattroff(window, COLOR_PAIR(Theme::Sidebar));
        return;
    }

    for (int row = 0; row < lh; ++row)
    {
        const int idx = scroll_y + row;
        if (idx >= static_cast<int>(visible.size()))
            break;

        const auto &v_item = visible[static_cast<std::size_t>(idx)];
        const auto &entry = *v_item.entry;
        const bool selected = (idx == selected_index);
        const bool is_open = entry.is_file && same_path(entry.entry_path, active_file);
        const bool on_clip = has_clipboard() && same_path(entry.entry_path, clipboard_path);
        const int screen_row = row + 1;

        short pair = Theme::SidebarFile;
        if (selected)
            pair = Theme::SidebarSelected;
        else if (on_clip)
            pair = Theme::SidebarHover;
        else if (entry.is_dir)
            pair = Theme::SidebarDir;
        else if (is_open)
            pair = Theme::SidebarHover;

        wattron(window, COLOR_PAIR(pair));
        if (selected || is_open || on_clip)
            mvwhline(window, screen_row, 0, ' ', width);

        int col = 1 + v_item.depth * 2;
        if (col >= width)
            col = width - 1;

        wmove(window, screen_row, col);

        if (entry.is_dir)
        {
            const bool is_expanded = expanded.count(path_key(entry.entry_path)) > 0;
            waddwstr(window, is_expanded ? Icons::chevron_down : Icons::chevron_right);
            waddwstr(window, L" ");
            waddwstr(window, is_expanded ? Icons::folder_opened : Icons::folder);
            waddwstr(window, L" ");
        }
        else
        {
            waddwstr(window, L"  ");
            waddwstr(window, Icons::for_file(entry.entry_path));
            waddwstr(window, L" ");
        }

        std::string name = entry.entry_path.filename().string();
        if (on_clip && clipboard_mode == SidebarClipboardMode::Cut)
            name = "✂ " + name;
        else if (on_clip && clipboard_mode == SidebarClipboardMode::Copy)
            name = "⧉ " + name;
        else if (is_open && !selected)
            name = "• " + name;

        const int used = getcurx(window);
        const int remain = std::max(0, width - used - 1);
        if (static_cast<int>(name.size()) > remain && remain > 1)
            name = name.substr(0, static_cast<std::size_t>(remain - 1)) + "…";
        wprintw(window, "%s", name.c_str());

        wattroff(window, COLOR_PAIR(pair));
    }

    // Scroll hint
    if (scroll_y > 0 || scroll_y + lh < static_cast<int>(visible.size()))
    {
        wattron(window, COLOR_PAIR(Theme::SidebarTitle));
        mvwprintw(window, height - 1, 1, "%d/%d", selected_index + 1, static_cast<int>(visible.size()));
        wattroff(window, COLOR_PAIR(Theme::SidebarTitle));
    }
}
