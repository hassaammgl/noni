#include <components/file_picker.hpp>
#include <ui/icons.hpp>
#include <ui/theme.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <string_view>


void FilePicker::refilter()
{
    matches.clear();

    if (query.empty() && !recent_.empty())
    {
        const fs::path root = index.root();
        for (const auto &p : recent_)
        {
            FuzzyMatch m;
            m.path = p;
            m.score = 10000;
            try
            {
                m.display = "[recent] " + (root.empty() ? p.string() : fs::relative(p, root).string());
            }
            catch (...)
            {
                m.display = "[recent] " + p.string();
            }
            if (m.display.empty() || m.display == "[recent] ")
                m.display = "[recent] " + p.string();
            matches.push_back(std::move(m));
            if (matches.size() >= 12)
                break;
        }
    }

    auto fuzzy = Fuzzy::filter(index.files(), index.root(), query, 300);
    for (auto &m : fuzzy)
    {
        // Skip duplicates already shown as recent when query empty.
        if (query.empty())
        {
            bool dup = false;
            for (const auto &r : matches)
            {
                if (r.path == m.path)
                {
                    dup = true;
                    break;
                }
            }
            if (dup)
                continue;
        }
        matches.push_back(std::move(m));
    }

    if (selected >= static_cast<int>(matches.size()))
        selected = matches.empty() ? 0 : static_cast<int>(matches.size()) - 1;
    if (selected < 0)
        selected = 0;
    ensure_selection_visible();
}

void FilePicker::set_recent(std::vector<fs::path> recent)
{
    recent_ = std::move(recent);
    if (active)
        refilter();
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

