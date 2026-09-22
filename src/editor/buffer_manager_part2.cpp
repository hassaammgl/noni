#include <editor/buffer_manager.hpp>

#include <utils/logger.hpp>
#include <utils/messages.hpp>

#include <algorithm>
#include <format>
#include <system_error>
#include <unordered_set>


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
