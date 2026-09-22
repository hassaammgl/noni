#include "sidebar_detail.hpp"

using namespace sidebar_detail;

bool Sidebar::create_file_here(const std::string &name)
{
    if (name.empty() || name == "." || name == "..")
        return false;

    const fs::path parent = get_create_directory();
    const fs::path target = (parent / name).lexically_normal();
    if (fs.exists(target))
        return false;

    if (!fs.create_file(target))
        return false;

    // Expand parent chain and refresh tree.
    expanded.insert(path_key(parent));
    if (parent != project_path)
    {
        fs::path walk = parent.parent_path();
        while (!walk.empty() && path_key(walk) != path_key(project_path))
        {
            expanded.insert(path_key(walk));
            if (walk == walk.parent_path())
                break;
            walk = walk.parent_path();
        }
    }

    ds.ensure_loaded(parent);
    // Soft refresh root to pick up new siblings without full async wipe.
    seen_version = 0;
    ds.scan_dirs();
    sync_cache();
    // ensure nested loaded after rescan
    ds.ensure_loaded(parent);
    for (const auto &p : expanded)
    {
        std::error_code ec;
        if (fs::is_directory(p, ec))
            ds.ensure_loaded(p);
    }
    sync_cache();
    select_path(target);
    return true;
}

bool Sidebar::create_folder_here(const std::string &name)
{
    if (name.empty() || name == "." || name == "..")
        return false;

    const fs::path parent = get_create_directory();
    const fs::path target = (parent / name).lexically_normal();
    if (fs.exists(target))
        return fs.is_directory(target);

    if (!fs.create_directory(target) && !fs.is_directory(target))
        return false;

    expanded.insert(path_key(parent));
    expanded.insert(path_key(target));
    seen_version = 0;
    ds.scan_dirs();
    sync_cache();
    ds.ensure_loaded(parent);
    ds.ensure_loaded(target);
    for (const auto &p : expanded)
    {
        std::error_code ec;
        if (fs::is_directory(p, ec))
            ds.ensure_loaded(p);
    }
    sync_cache();
    select_path(target);
    return true;
}

bool Sidebar::rename_selected(const std::string &new_name)
{
    const fs::path selected = get_selected_path();
    if (selected.empty() || new_name.empty() || new_name == "." || new_name == "..")
        return false;

    // Keep same parent unless user typed a path.
    fs::path target;
    if (new_name.find('/') != std::string::npos)
        target = (selected.parent_path() / new_name).lexically_normal();
    else
        target = selected.parent_path() / new_name;

    if (path_key(target) == path_key(selected))
        return true;
    if (fs.exists(target))
        return false;

    if (!fs.rename_file(selected, target))
        return false;

    seen_version = 0;
    ds.scan_dirs();
    sync_cache();
    const fs::path parent = target.parent_path();
    ds.ensure_loaded(parent.empty() ? project_path : parent);
    for (const auto &p : expanded)
    {
        std::error_code ec;
        if (fs::is_directory(p, ec))
            ds.ensure_loaded(p);
    }
    sync_cache();
    select_path(target);
    return true;
}

bool Sidebar::delete_selected()
{
    return delete_path(get_selected_path());
}

bool Sidebar::delete_path(const fs::path &selected)
{
    if (selected.empty() || path_key(selected) == path_key(project_path))
        return false;

    bool ok = false;
    if (fs.is_directory(selected))
        ok = fs.delete_directory(selected);
    else
        ok = fs.delete_file(selected);

    if (!ok)
        return false;

    expanded.erase(path_key(selected));
    seen_version = 0;
    ds.scan_dirs();
    sync_cache();
    for (const auto &p : expanded)
    {
        std::error_code ec;
        if (fs::is_directory(p, ec))
            ds.ensure_loaded(p);
    }
    sync_cache();
    clamp_selection(static_cast<int>(visible_rows().size()));
    return true;
}

void Sidebar::refresh_tree_keep_expanded()
{
    seen_version = 0;
    ds.scan_dirs();
    sync_cache();
    for (const auto &p : expanded)
    {
        std::error_code ec;
        if (fs::is_directory(p, ec))
            ds.ensure_loaded(p);
    }
    sync_cache();
}

bool Sidebar::is_under(const fs::path &parent, const fs::path &child) const
{
    const std::string p = path_key(parent);
    const std::string c = path_key(child);
    if (p.empty() || c.empty())
        return false;
    if (c == p)
        return true;
    return c.size() > p.size() && c.compare(0, p.size(), p) == 0 &&
           (p.back() == '/' || c[p.size()] == '/');
}
