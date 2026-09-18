#include <editor/buffer_manager.hpp>

#include <utils/logger.hpp>
#include <utils/messages.hpp>

#include <algorithm>
#include <format>
#include <system_error>
#include <unordered_set>

Buffer *BufferManager::create_buffer()
{
    buffers.push_back(std::make_unique<Buffer>());
    return buffers.back().get();
}

void BufferManager::destroy_buffer(Buffer *buffer)
{
    if (!buffer)
        return;

    buffers.erase(
        std::remove_if(
            buffers.begin(),
            buffers.end(),
            [buffer](const std::unique_ptr<Buffer> &b) { return b.get() == buffer; }),
        buffers.end());
}

bool BufferManager::buffer_referenced(Buffer *buffer) const
{
    if (!buffer)
        return false;
    for (const auto &tab : tabs)
    {
        for (Window *w : tab.layout.leaves())
        {
            if (w && w->has_buffer() && &w->buffer() == buffer)
                return true;
        }
    }
    return false;
}

int BufferManager::find_tab_by_path(const fs::path &path) const
{
    if (path.empty())
        return -1;

    for (std::size_t i = 0; i < tabs.size(); ++i)
    {
        for (Window *w : tabs[i].layout.leaves())
        {
            if (w && w->has_buffer() && w->buffer().get_buffer_path() == path)
                return static_cast<int>(i);
        }
    }

    return -1;
}

void BufferManager::open_untitled()
{
    Buffer *buf = create_buffer();
    buf->load();

    EditorTab tab;
    tab.layout.reset(buf);
    tab.mode = EditorMode::Normal;
    tabs.push_back(std::move(tab));
    active_index = static_cast<int>(tabs.size()) - 1;
}

void BufferManager::open_file(const fs::path &path)
{
    const int existing = find_tab_by_path(path);
    if (existing >= 0)
    {
        active_index = existing;
        Logger::info(std::format("Switched to open tab: {}", path.string()));
        return;
    }

    Buffer *buf = create_buffer();
    buf->set_buffer_path(path);

    EditorTab tab;
    tab.layout.reset(buf);
    tab.mode = EditorMode::Normal;
    tabs.push_back(std::move(tab));
    active_index = static_cast<int>(tabs.size()) - 1;

    if (buf->has_load_error())
    {
        Messages::error(std::format(
            "Failed to open {}: {}",
            path.string(),
            buf->get_load_error()));
        Logger::warning(std::format("Opened tab with load error: {}", path.string()));
    }
    else
    {
        Logger::info(std::format("Opened new tab: {}", path.string()));
    }
}

bool BufferManager::close_active(bool force)
{
    if (active_index < 0 || active_index >= static_cast<int>(tabs.size()))
        return true;

    EditorTab &tab = tabs[static_cast<std::size_t>(active_index)];
    if (!force && tab.buffer().is_dirty())
        return false;

    std::unordered_set<Buffer *> owned;
    for (Window *w : tab.layout.leaves())
    {
        if (w && w->has_buffer())
            owned.insert(&w->buffer());
    }

    tabs.erase(tabs.begin() + active_index);

    for (Buffer *b : owned)
    {
        if (!buffer_referenced(b))
            destroy_buffer(b);
    }

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

Buffer *BufferManager::find_buffer_by_path(const fs::path &path)
{
    if (path.empty())
        return nullptr;

    std::error_code ec;
    fs::path want = fs::weakly_canonical(path, ec);
    if (ec)
        want = path;

    for (auto &tab : tabs)
    {
        for (Window *w : tab.layout.leaves())
        {
            if (!w || !w->has_buffer())
                continue;
            const fs::path bp = w->buffer().get_buffer_path();
            if (bp.empty())
                continue;
            if (bp == path || bp == want)
                return &w->buffer();
            fs::path bcanon = fs::weakly_canonical(bp, ec);
            if (!ec && (bcanon == want || bcanon == path))
                return &w->buffer();
        }
    }
    return nullptr;
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
