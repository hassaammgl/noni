#include "sidebar_detail.hpp"

using namespace sidebar_detail;

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
    case '\r':
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
