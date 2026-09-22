#include "sidebar_detail.hpp"

using namespace sidebar_detail;

fs::path Sidebar::get_project_path() const
{
    return project_path;
}

void Sidebar::set_project_path(const fs::path &path)
{
    std::error_code ec;
    project_path = fs::weakly_canonical(path, ec);
    if (ec)
        project_path = path.lexically_normal();

    Logger::info(std::format("Sidebar project path set: {}", project_path.string()));
    ds.set_project_path(project_path);
    seen_version = 0;
    cached_entries.clear();
    ds.scan_dirs_async();
    selected_index = 0;
    scroll_y = 0;
    expanded.clear();
}

void Sidebar::refresh()
{
    if (project_path.empty())
        return;
    seen_version = 0;
    ds.scan_dirs_async();
    Logger::info("Sidebar refreshed");
}

void Sidebar::set_focused(bool value)
{
    focused = value;
}

void Sidebar::set_active_file(const fs::path &path)
{
    active_file = path;
}

void Sidebar::reveal_path(const fs::path &path)
{
    if (path.empty() || project_path.empty())
        return;

    sync_cache();

    std::error_code ec;
    fs::path current = fs::weakly_canonical(path, ec);
    if (ec)
        current = path.lexically_normal();

    // Expand all parents under project root (load children lazily as we go).
    fs::path walk = current.parent_path();
    std::vector<fs::path> parents;
    while (!walk.empty() && walk != walk.root_path())
    {
        if (path_key(walk).rfind(path_key(project_path), 0) != 0 &&
            path_key(walk) != path_key(project_path))
            break;
        if (path_key(walk) != path_key(project_path))
            parents.push_back(walk);
        if (path_key(walk) == path_key(project_path))
            break;
        walk = walk.parent_path();
    }

    std::reverse(parents.begin(), parents.end());
    for (const auto &p : parents)
    {
        ds.ensure_loaded(p);
        expanded.insert(path_key(p));
    }
    sync_cache();

    active_file = current;
    const auto visible = visible_rows();
    for (std::size_t i = 0; i < visible.size(); ++i)
    {
        if (same_path(visible[i].entry->entry_path, current))
        {
            selected_index = static_cast<int>(i);
            ensure_visible(list_height());
            return;
        }
    }
}
