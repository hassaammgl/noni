#include <components/file_picker.hpp>
#include <ui/icons.hpp>
#include <ui/theme.hpp>

#include <algorithm>

void FilePicker::refilter()
{
    matches = Fuzzy::filter(index.files(), index.root(), query, 300);
    if (selected >= static_cast<int>(matches.size()))
        selected = matches.empty() ? 0 : static_cast<int>(matches.size()) - 1;
    if (selected < 0)
        selected = 0;
    ensure_selection_visible();
}

void FilePicker::ensure_selection_visible()
{
    const int list_height = std::max(1, height - 3);
    if (selected < scroll_y)
        scroll_y = selected;
    if (selected >= scroll_y + list_height)
        scroll_y = selected - list_height + 1;
    if (scroll_y < 0)
        scroll_y = 0;
}

void FilePicker::draw()
{
    if (!window || !active)
        return;

    werase(window);
    leaveok(window, FALSE);
    wbkgd(window, COLOR_PAIR(Theme::Popup));
    wattron(window, COLOR_PAIR(Theme::Popup));
    for (int row = 0; row < height; ++row)
        mvwhline(window, row, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::Popup));

    wattron(window, COLOR_PAIR(Theme::PopupBorder));
    mvwhline(window, 0, 0, ACS_HLINE, width);
    mvwhline(window, height - 2, 0, ACS_HLINE, width);
    wattroff(window, COLOR_PAIR(Theme::PopupBorder));

    wattron(window, COLOR_PAIR(Theme::Popup));
    if (index.is_indexing() && index.file_count() == 0)
        mvwprintw(window, 0, 2, " file search  indexing… ");
    else
        mvwprintw(window, 0, 2, " file search  %zu/%zu%s ",
                  matches.size(),
                  index.file_count(),
                  index.is_indexing() ? "…" : "");
    wattroff(window, COLOR_PAIR(Theme::Popup));

    const int list_height = std::max(1, height - 3);
    for (int row = 0; row < list_height; ++row)
    {
        const int idx = scroll_y + row;
        if (idx >= static_cast<int>(matches.size()))
            break;

        const bool is_sel = (idx == selected);
        const short pair = is_sel ? Theme::PopupSelected : Theme::Popup;
        wattron(window, COLOR_PAIR(pair));
        if (is_sel)
            wattron(window, A_BOLD);
        mvwhline(window, row + 1, 0, ' ', width);

        const auto &m = matches[static_cast<std::size_t>(idx)];
        mvwaddwstr(window, row + 1, 1, Icons::for_file(m.path));
        mvwprintw(window, row + 1, 4, "%.*s", width - 5, m.display.c_str());

        if (is_sel)
            wattroff(window, A_BOLD);
        wattroff(window, COLOR_PAIR(pair));
    }

    wattron(window, COLOR_PAIR(Theme::InputFocus));
    mvwhline(window, height - 1, 0, ' ', width);
    mvwprintw(window, height - 1, 0, "> %s", query.c_str());
    wattroff(window, COLOR_PAIR(Theme::InputFocus));
    wmove(window, height - 1, 2 + static_cast<int>(query.size()));
}

void FilePicker::warm(const fs::path &project_root)
{
    if (project_root.empty())
        return;

    if (index.root() != project_root)
    {
        index.set_root(project_root);
        index.rebuild_async();
        seen_version = index.version();
        return;
    }

    if (index.file_count() == 0 && !index.is_indexing())
    {
        index.rebuild_async();
        seen_version = index.version();
    }
}

void FilePicker::reindex()
{
    if (index.root().empty())
        return;
    index.rebuild_async();
    seen_version = index.version();
}

void FilePicker::open(const fs::path &project_root)
{
    active = true;
    query.clear();
    selected = 0;
    scroll_y = 0;

    if (index.root() != project_root)
    {
        index.set_root(project_root);
        index.rebuild_async();
    }
    else if (index.file_count() == 0 && !index.is_indexing())
    {
        index.rebuild_async();
    }

    seen_version = index.version();
    refilter();
}

void FilePicker::close()
{
    active = false;
    query.clear();
    selected = 0;
    scroll_y = 0;
    matches.clear();
}

bool FilePicker::is_active() const
{
    return active;
}

bool FilePicker::poll()
{
    const auto v = index.version();
    if (v == seen_version)
        return false;
    seen_version = v;
    if (active)
        refilter();
    return true;
}

void FilePicker::handle_input(int key)
{
    if (!active)
        return;

    switch (key)
    {
    case KEY_UP:
    case 16: // Ctrl+P
        if (selected > 0)
        {
            --selected;
            ensure_selection_visible();
        }
        break;
    case KEY_DOWN:
    case 14: // Ctrl+N
        if (selected + 1 < static_cast<int>(matches.size()))
        {
            ++selected;
            ensure_selection_visible();
        }
        break;
    case KEY_PPAGE:
        selected = std::max(0, selected - std::max(1, height - 3));
        ensure_selection_visible();
        break;
    case KEY_NPAGE:
        selected = std::min(
            static_cast<int>(matches.size()) - 1,
            selected + std::max(1, height - 3));
        if (selected < 0)
            selected = 0;
        ensure_selection_visible();
        break;
    case KEY_HOME:
        selected = 0;
        ensure_selection_visible();
        break;
    case KEY_END:
        selected = matches.empty() ? 0 : static_cast<int>(matches.size()) - 1;
        ensure_selection_visible();
        break;
    case KEY_BACKSPACE:
    case 127:
    case 8:
        if (!query.empty())
        {
            query.pop_back();
            selected = 0;
            refilter();
        }
        break;
    case 21: // Ctrl+U clear query
        query.clear();
        selected = 0;
        refilter();
        break;
    default:
        if (key >= 32 && key <= 126)
        {
            query.push_back(static_cast<char>(key));
            selected = 0;
            refilter();
        }
        break;
    }
}

bool FilePicker::take_selection(fs::path &out_path)
{
    if (!active || matches.empty() || selected < 0 ||
        selected >= static_cast<int>(matches.size()))
        return false;

    out_path = matches[static_cast<std::size_t>(selected)].path;
    return true;
}

const std::string &FilePicker::get_query() const
{
    return query;
}
