#include <editor/buffer_manager.hpp>

#include <utils/logger.hpp>

#include <format>

int BufferManager::find_by_path(const fs::path &path) const
{
    if (path.empty())
        return -1;

    for (std::size_t i = 0; i < tabs.size(); ++i)
    {
        if (tabs[i].buffer.get_buffer_path() == path)
            return static_cast<int>(i);
    }

    return -1;
}

void BufferManager::open_untitled()
{
    EditorTab tab;
    tab.buffer.load();
    tabs.push_back(std::move(tab));
    active_index = static_cast<int>(tabs.size()) - 1;
}

void BufferManager::open_file(const fs::path &path)
{
    const int existing = find_by_path(path);
    if (existing >= 0)
    {
        active_index = existing;
        Logger::info(std::format("Switched to open tab: {}", path.string()));
        return;
    }

    EditorTab tab;
    tab.buffer.set_buffer_path(path);
    tab.cursor = {.line = 0, .column = 0};
    tab.scroll_y = 0;
    tab.scroll_x = 0;
    tab.mode = EditorMode::Normal;
    tabs.push_back(std::move(tab));
    active_index = static_cast<int>(tabs.size()) - 1;
    Logger::info(std::format("Opened new tab: {}", path.string()));
}

bool BufferManager::close_active(bool force)
{
    if (active_index < 0 || active_index >= static_cast<int>(tabs.size()))
        return true;

    if (!force && tabs[static_cast<std::size_t>(active_index)].buffer.is_dirty())
        return false;

    tabs.erase(tabs.begin() + active_index);

    if (tabs.empty())
    {
        active_index = -1;
        return true;
    }

    if (active_index >= static_cast<int>(tabs.size()))
        active_index = static_cast<int>(tabs.size()) - 1;

    return true;
}

void BufferManager::switch_to(int index)
{
    if (index < 0 || index >= static_cast<int>(tabs.size()))
        return;

    active_index = index;
}

void BufferManager::next_tab()
{
    if (tabs.size() <= 1)
        return;

    active_index = (active_index + 1) % static_cast<int>(tabs.size());
}

void BufferManager::prev_tab()
{
    if (tabs.size() <= 1)
        return;

    active_index = (active_index - 1 + static_cast<int>(tabs.size())) % static_cast<int>(tabs.size());
}

EditorTab &BufferManager::active()
{
    return tabs[static_cast<std::size_t>(active_index)];
}

const EditorTab &BufferManager::active() const
{
    return tabs[static_cast<std::size_t>(active_index)];
}

int BufferManager::get_active_index() const
{
    return active_index;
}

const std::vector<EditorTab> &BufferManager::get_tabs() const
{
    return tabs;
}

bool BufferManager::has_tabs() const
{
    return active_index >= 0 && !tabs.empty();
}

std::size_t BufferManager::size() const
{
    return tabs.size();
}
