#include "sidebar_detail.hpp"

using namespace sidebar_detail;

bool Sidebar::cut_selected()
{
    const fs::path selected = get_selected_path();
    if (selected.empty() || path_key(selected) == path_key(project_path))
        return false;
    clipboard_path = selected;
    clipboard_mode = SidebarClipboardMode::Cut;
    return true;
}

bool Sidebar::copy_selected()
{
    const fs::path selected = get_selected_path();
    if (selected.empty() || path_key(selected) == path_key(project_path))
        return false;
    clipboard_path = selected;
    clipboard_mode = SidebarClipboardMode::Copy;
    return true;
}

void Sidebar::clear_clipboard()
{
    clipboard_path.clear();
    clipboard_mode = SidebarClipboardMode::None;
}

bool Sidebar::has_clipboard() const
{
    return clipboard_mode != SidebarClipboardMode::None && !clipboard_path.empty();
}

SidebarClipboardMode Sidebar::get_clipboard_mode() const
{
    return clipboard_mode;
}

fs::path Sidebar::get_clipboard_path() const
{
    return clipboard_path;
}

bool Sidebar::paste_here(fs::path *moved_from, fs::path *moved_to)
{
    if (!has_clipboard() || !fs.exists(clipboard_path))
    {
        clear_clipboard();
        return false;
    }

    const fs::path dest_dir = get_create_directory();
    const fs::path dest = (dest_dir / clipboard_path.filename()).lexically_normal();

    // Don't move/copy onto itself or into its own subtree.
    if (path_key(dest) == path_key(clipboard_path))
        return false;
    if (fs.is_directory(clipboard_path) && is_under(clipboard_path, dest_dir))
        return false;
    if (fs.exists(dest))
        return false;

    const fs::path from = clipboard_path;
    const auto mode = clipboard_mode;
    bool ok = false;

    if (mode == SidebarClipboardMode::Cut)
    {
        ok = fs.rename_file(from, dest);
        if (ok)
            clear_clipboard();
    }
    else if (mode == SidebarClipboardMode::Copy)
    {
        try
        {
            const auto options = fs::copy_options::recursive | fs::copy_options::overwrite_existing;
            fs::copy(from, dest, options);
            ok = fs.exists(dest);
        }
        catch (const std::exception &e)
        {
            Logger::error(std::format("Copy failed: {}", e.what()));
            ok = false;
        }
        // Keep clipboard for multiple pastes on copy.
    }

    if (!ok)
        return false;

    expanded.insert(path_key(dest_dir));
    if (fs.is_directory(dest))
        expanded.insert(path_key(dest));

    refresh_tree_keep_expanded();
    select_path(dest);

    if (moved_from)
        *moved_from = from;
    if (moved_to)
        *moved_to = dest;
    return true;
}

void Sidebar::toggle_expand(const fs::path &path)
{
    const auto key = path_key(path);
    if (expanded.count(key))
    {
        expanded.erase(key);
        Logger::info(std::format("Collapsed folder: {}", key));
    }
    else
    {
        ds.ensure_loaded(path);
        sync_cache();
        expanded.insert(key);
        Logger::info(std::format("Expanded folder: {}", key));
    }
}

void Sidebar::collect_visible(const std::vector<ScanedEntry> &entries, int depth, std::vector<VisibleRow> &out) const
{
    for (const auto &e : entries)
    {
        out.push_back({&e, depth});
        if (e.is_dir && e.children_loaded && expanded.count(path_key(e.entry_path)))
            collect_visible(e.inner_entries, depth + 1, out);
    }
}
