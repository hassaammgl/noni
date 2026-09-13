#include <components/sidebar.hpp>
#include <ui/icons.hpp>
#include <ui/theme.hpp>

#include <algorithm>
#include <format>

namespace
{
    std::string path_key(const fs::path &path)
    {
        std::error_code ec;
        const fs::path abs = fs::weakly_canonical(path, ec);
        return ec ? path.lexically_normal().string() : abs.string();
    }

    bool same_path(const fs::path &a, const fs::path &b)
    {
        if (a.empty() || b.empty())
            return false;
        return path_key(a) == path_key(b);
    }
}

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

SidebarAction Sidebar::handle_input(int key)
{
    sync_cache();
    auto visible = visible_rows();
    clamp_selection(static_cast<int>(visible.size()));
    if (visible.empty())
        return SidebarAction::None;

    const int lh = list_height();
    const int prev = selected_index;

    switch (key)
    {
    case KEY_UP:
    case 'k':
        if (selected_index > 0)
            --selected_index;
        break;
    case KEY_DOWN:
    case 'j':
        if (selected_index + 1 < static_cast<int>(visible.size()))
            ++selected_index;
        break;
    case KEY_PPAGE:
        selected_index = std::max(0, selected_index - std::max(1, lh));
        break;
    case KEY_NPAGE:
        selected_index = std::min(
            static_cast<int>(visible.size()) - 1,
            selected_index + std::max(1, lh));
        break;
    case 'g':
    case KEY_HOME:
        selected_index = 0;
        break;
    case 'G':
    case KEY_END:
        selected_index = static_cast<int>(visible.size()) - 1;
        break;
    case KEY_LEFT:
    case 'h':
    {
        const auto &row = visible[static_cast<std::size_t>(selected_index)];
        const fs::path path = row.entry->entry_path;
        if (row.entry->is_dir && expanded.count(path_key(path)))
        {
            toggle_expand(path);
        }
        else if (row.depth > 0)
        {
            // Jump to parent row
            for (int i = selected_index - 1; i >= 0; --i)
            {
                if (visible[static_cast<std::size_t>(i)].depth < row.depth)
                {
                    selected_index = i;
                    break;
                }
            }
        }
        break;
    }
    case KEY_RIGHT:
    case 'l':
    {
        const auto &row = visible[static_cast<std::size_t>(selected_index)];
        if (row.entry->is_dir)
        {
            if (!expanded.count(path_key(row.entry->entry_path)))
                toggle_expand(row.entry->entry_path);
            else if (selected_index + 1 < static_cast<int>(visible.size()))
                ++selected_index;
        }
        else if (row.entry->is_file)
        {
            ensure_visible(lh);
            return SidebarAction::OpenFile;
        }
        break;
    }
    case '\n':
    case KEY_ENTER:
    case 'o':
    {
        const auto &row = visible[static_cast<std::size_t>(selected_index)];
        if (row.entry->is_dir)
        {
            toggle_expand(row.entry->entry_path);
            break;
        }
        if (row.entry->is_file)
        {
            ensure_visible(lh);
            return SidebarAction::OpenFile;
        }
        break;
    }
    case ' ':
    {
        const auto &row = visible[static_cast<std::size_t>(selected_index)];
        if (row.entry->is_dir)
            toggle_expand(row.entry->entry_path);
        break;
    }
    case 'a':
        return SidebarAction::AddFile;
    case 'A':
        return SidebarAction::AddFolder;
    case 'r':
        return SidebarAction::Rename;
    case 'd':
        return SidebarAction::Delete;
    case 'x':
        return SidebarAction::Cut;
    case 'c':
        return SidebarAction::Copy;
    case 'p':
        return SidebarAction::Paste;
    case 'R':
        refresh();
        visible = visible_rows();
        clamp_selection(static_cast<int>(visible.size()));
        break;
    case 'q':
        return SidebarAction::FocusEditor;
    default:
        break;
    }

    visible = visible_rows();
    clamp_selection(static_cast<int>(visible.size()));
    ensure_visible(lh);

    if (selected_index != prev && !visible.empty())
    {
        Logger::debug(std::format(
            "Sidebar selection: {} -> {} ({})",
            prev,
            selected_index,
            visible[static_cast<std::size_t>(selected_index)].entry->entry_path.string()));
    }

    return SidebarAction::None;
}

fs::path Sidebar::get_selected_path() const
{
    const auto visible = visible_rows();
    if (visible.empty() || selected_index < 0 ||
        selected_index >= static_cast<int>(visible.size()))
        return {};
    return visible[static_cast<std::size_t>(selected_index)].entry->entry_path;
}

bool Sidebar::is_selected_dir() const
{
    const auto visible = visible_rows();
    if (visible.empty() || selected_index < 0 ||
        selected_index >= static_cast<int>(visible.size()))
        return false;
    return visible[static_cast<std::size_t>(selected_index)].entry->is_dir;
}

fs::path Sidebar::get_create_directory() const
{
    const fs::path selected = get_selected_path();
    if (selected.empty())
        return project_path;

    if (is_selected_dir())
        return selected;
    return selected.parent_path().empty() ? project_path : selected.parent_path();
}

void Sidebar::select_path(const fs::path &path)
{
    if (path.empty())
        return;
    reveal_path(path);
}

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
