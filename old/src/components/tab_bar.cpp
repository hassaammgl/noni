#include <components/tab_bar.hpp>
#include <ui/icons.hpp>
#include <ui/theme.hpp>

#include <algorithm>
#include <cwchar>
#include <string>

namespace
{
    int display_width(const wchar_t *text)
    {
        if (!text)
            return 0;

        int total = 0;
        for (const wchar_t *p = text; *p; ++p)
        {
            const int w = wcwidth(*p);
            total += (w > 0) ? w : 1;
        }
        return total;
    }

    int display_width(const std::string &text)
    {
        return static_cast<int>(text.size());
    }

    std::string truncate_name(std::string name, int max_chars)
    {
        if (max_chars < 4)
            return name.substr(0, static_cast<std::size_t>(std::max(max_chars, 0)));
        if (static_cast<int>(name.size()) <= max_chars)
            return name;
        return name.substr(0, static_cast<std::size_t>(max_chars - 1)) + "…";
    }
}

void TabBar::set_manager(const BufferManager *new_manager)
{
    manager = new_manager;
}

void TabBar::draw()
{
    if (!window || !manager || !manager->has_tabs())
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::TabInactive));
    wattron(window, COLOR_PAIR(Theme::TabInactive));
    mvwhline(window, 0, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::TabInactive));

    const int active = manager->get_active_index();
    const auto &tabs = manager->get_tabs();
    const int count = static_cast<int>(tabs.size());
    if (count <= 0)
        return;

    // Keep active tab on screen when many tabs overflow.
    int start = 0;
    if (active > 0)
    {
        int estimated = 0;
        for (int i = active; i >= 0; --i)
        {
            const int name_w = std::min(24, display_width(tabs[static_cast<std::size_t>(i)].display_name()));
            const int tab_w = 2 + 2 + name_w + 2 + 2; // pad + icon + name + dirty/close + pad
            if (estimated + tab_w > width && i != active)
            {
                start = i + 1;
                break;
            }
            estimated += tab_w;
        }
    }

    int x = 0;
    for (int i = start; i < count; ++i)
    {
        const EditorTab &tab = tabs[static_cast<std::size_t>(i)];
        const bool is_active = (i == active);
        const bool dirty = tab.buffer().is_dirty();
        const fs::path path = tab.buffer().get_buffer_path();
        const wchar_t *icon = Icons::for_file(path.empty() ? fs::path("untitled.txt") : path);

        std::string name = truncate_name(tab.display_name(), 22);
        const int icon_w = std::max(1, display_width(icon));
        // " icon name  ●  × " style padding
        const int dirty_w = dirty ? 2 : 0;
        const int close_w = is_active ? 2 : 0;
        const int tab_w = 1 + icon_w + 1 + display_width(name) + 1 + dirty_w + close_w + 1;

        if (x + tab_w > width)
            break;

        short pair = Theme::TabInactive;
        if (is_active)
            pair = Theme::TabActive;
        else if (dirty)
            pair = Theme::TabModified;

        wattron(window, COLOR_PAIR(pair));
        if (is_active)
            wattron(window, A_BOLD);
        mvwhline(window, 0, x, ' ', tab_w);

        int cx = x + 1;
        mvwaddwstr(window, 0, cx, icon);
        cx += icon_w + 1;
        mvwprintw(window, 0, cx, "%s", name.c_str());
        cx += display_width(name) + 1;

        if (dirty)
        {
            mvwaddwstr(window, 0, cx, L"●");
            cx += 2;
        }

        if (is_active)
        {
            mvwaddwstr(window, 0, cx, Icons::close);
            cx += 2;
        }

        if (is_active)
            wattroff(window, A_BOLD);
        wattroff(window, COLOR_PAIR(pair));

        x += tab_w;

        // VS Code-like gap between tabs
        if (i + 1 < count && x < width)
        {
            wattron(window, COLOR_PAIR(Theme::TabInactive));
            mvwaddch(window, 0, x, ' ');
            wattroff(window, COLOR_PAIR(Theme::TabInactive));
            ++x;
        }
    }

    // Remaining bar stays inactive so active tab "connects" to editor.
    if (x < width)
    {
        wattron(window, COLOR_PAIR(Theme::TabInactive));
        mvwhline(window, 0, x, ' ', width - x);
        wattroff(window, COLOR_PAIR(Theme::TabInactive));
    }
}
