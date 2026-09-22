#include <components/file_picker.hpp>
#include <ui/icons.hpp>
#include <ui/theme.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <string_view>


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
        if (selected > 0)
        {
            --selected;
            ensure_selection_visible();
        }
        break;
    case KEY_DOWN:
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
            TextMetrics::pop_codepoint(query);
            selected = 0;
            refilter();
        }
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

void FilePicker::insert_utf8(std::string_view utf8)
{
    if (!active || utf8.empty())
        return;
    query.append(utf8.data(), utf8.size());
    selected = 0;
    refilter();
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
