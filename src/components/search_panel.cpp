#include <components/search_panel.hpp>
#include <ui/theme.hpp>
#include <utils/str.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <format>
#include <string_view>


int SearchPanel::list_top() const
{
    // title + query + replace + status = 4
    return 4;
}

int SearchPanel::list_height() const
{
    return std::max(0, height - list_top());
}

void SearchPanel::sync_results()
{
    const auto v = engine.version();
    if (v == seen_version)
        return;
    seen_version = v;
    cached = engine.results();
    if (selected >= static_cast<int>(cached.size()))
        selected = cached.empty() ? 0 : static_cast<int>(cached.size()) - 1;
    if (selected < 0)
        selected = 0;
}

void SearchPanel::ensure_selection_visible(int list_h)
{
    if (list_h <= 0)
    {
        scroll_y = 0;
        return;
    }
    if (selected < scroll_y)
        scroll_y = selected;
    if (selected >= scroll_y + list_h)
        scroll_y = selected - list_h + 1;
    if (scroll_y < 0)
        scroll_y = 0;
}

void SearchPanel::run_search()
{
    engine.search_async(query, opts);
    seen_version = 0;
    selected = 0;
    scroll_y = 0;
}

void SearchPanel::draw()
{
    if (!window || height <= 0 || width <= 0)
        return;

    sync_results();
    const int lh = list_height();
    ensure_selection_visible(lh);

    werase(window);
    leaveok(window, field != SearchField::Query && field != SearchField::Replace);
    wbkgd(window, COLOR_PAIR(Theme::Sidebar));
    wattron(window, COLOR_PAIR(Theme::Sidebar));
    for (int r = 0; r < height; ++r)
        mvwhline(window, r, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::Sidebar));

    // Title
    const short title = focused ? Theme::SidebarSelected : Theme::SidebarTitle;
    wattron(window, COLOR_PAIR(title));
    mvwhline(window, 0, 0, ' ', width);
    mvwprintw(window, 0, 1, "SEARCH");
    // toggles
    mvwprintw(
        window,
        0,
        std::max(8, width - 12),
        "%s%s%s",
        opts.match_case ? "Aa" : "aa",
        opts.whole_word ? " W" : " w",
        opts.use_regex ? " .*" : " .*");
    wattroff(window, COLOR_PAIR(title));

    auto draw_field = [&](int row, const char *label, const std::string &value, bool active) {
        const short pair = active ? Theme::InputFocus : Theme::Input;
        wattron(window, COLOR_PAIR(pair));
        mvwhline(window, row, 0, ' ', width);
        std::string shown = std::string(label) + value;
        if (static_cast<int>(shown.size()) > width && width > 1)
            shown = shown.substr(0, static_cast<std::size_t>(width - 1));
        mvwprintw(window, row, 0, "%s", shown.c_str());
        wattroff(window, COLOR_PAIR(pair));
        if (active)
            wmove(window, row, std::min(width - 1, static_cast<int>(std::string(label).size() + value.size())));
    };

    draw_field(1, "> ", query, focused && field == SearchField::Query);
    draw_field(2, "↔ ", replace_text, focused && field == SearchField::Replace);

    wattron(window, COLOR_PAIR(Theme::Dim));
    std::string st = engine.is_searching() ? "Searching…" : engine.status();
    if (st.empty())
        st = "Enter to search · F6/F7/F8 toggles";
    if (static_cast<int>(st.size()) > width && width > 1)
        st = st.substr(0, static_cast<std::size_t>(width - 1));
    mvwprintw(window, 3, 1, "%s", st.c_str());
    wattroff(window, COLOR_PAIR(Theme::Dim));

    fs::path last_path;
    for (int row = 0; row < lh; ++row)
    {
        const int idx = scroll_y + row;
        if (idx >= static_cast<int>(cached.size()))
            break;

        const auto &m = cached[static_cast<std::size_t>(idx)];
        const int screen = list_top() + row;
        const bool sel = focused && field == SearchField::Results && idx == selected;

        if (m.path != last_path)
        {
            last_path = m.path;
            // file header line uses same row as first match — show path prefix on match line
        }

        short pair = Theme::SidebarFile;
        if (sel)
            pair = Theme::SidebarSelected;
        else if (m.path != (idx > 0 ? cached[static_cast<std::size_t>(idx - 1)].path : fs::path{}))
            pair = Theme::SidebarDir;

        wattron(window, COLOR_PAIR(pair));
        if (sel)
            mvwhline(window, screen, 0, ' ', width);

        std::string line;
        const bool new_file = (idx == 0) ||
            cached[static_cast<std::size_t>(idx - 1)].path != m.path;
        if (new_file)
        {
            std::string rel = m.path.filename().string();
            line = std::format("{}:", rel);
        }
        else
        {
            line = std::format("  {}:{}", m.line + 1, m.preview);
        }

        // Always show line content; for new_file also show preview
        if (new_file)
            line = std::format("{}:{}:{}", m.path.filename().string(), m.line + 1, m.preview);

        if (static_cast<int>(line.size()) > width && width > 1)
            line = line.substr(0, static_cast<std::size_t>(width - 1));
        mvwprintw(window, screen, 0, "%s", line.c_str());
        wattroff(window, COLOR_PAIR(pair));
    }
}

void SearchPanel::set_root(const fs::path &root)
{
    engine.set_root(root);
}

