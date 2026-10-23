#include <extensions/editor_events.hpp>

#include <algorithm>

std::uint64_t EditorEvents::on_buffer_opened(BufferFn fn, std::string owner)
{
    const auto id = next_id_++;
    opened_.push_back(Sub<BufferFn>{id, std::move(owner), std::move(fn)});
    return id;
}

std::uint64_t EditorEvents::on_buffer_changed(BufferChangeFn fn, std::string owner)
{
    const auto id = next_id_++;
    changed_.push_back(Sub<BufferChangeFn>{id, std::move(owner), std::move(fn)});
    return id;
}

std::uint64_t EditorEvents::on_buffer_saved(BufferFn fn, std::string owner)
{
    const auto id = next_id_++;
    saved_.push_back(Sub<BufferFn>{id, std::move(owner), std::move(fn)});
    return id;
}

std::uint64_t EditorEvents::on_buffer_closed(BufferFn fn, std::string owner)
{
    const auto id = next_id_++;
    closed_.push_back(Sub<BufferFn>{id, std::move(owner), std::move(fn)});
    return id;
}

void EditorEvents::unsubscribe(std::uint64_t id)
{
    auto drop = [id](auto &list) {
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [id](const auto &s) { return s.id == id; }),
                   list.end());
    };
    drop(opened_);
    drop(changed_);
    drop(saved_);
    drop(closed_);
}

void EditorEvents::unsubscribe_owner(const std::string &owner)
{
    auto drop = [&owner](auto &list) {
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [&owner](const auto &s) { return s.owner == owner; }),
                   list.end());
    };
    drop(opened_);
    drop(changed_);
    drop(saved_);
    drop(closed_);
}

void EditorEvents::emit_buffer_opened(Buffer &b)
{
    auto copy = opened_;
    for (auto &s : copy)
    {
        if (s.fn)
            s.fn(b);
    }
}

void EditorEvents::emit_buffer_changed(Buffer &b, const TextChange &ch)
{
    auto copy = changed_;
    for (auto &s : copy)
    {
        if (s.fn)
            s.fn(b, ch);
    }
}

void EditorEvents::emit_buffer_saved(Buffer &b)
{
    auto copy = saved_;
    for (auto &s : copy)
    {
        if (s.fn)
            s.fn(b);
    }
}

void EditorEvents::emit_buffer_closed(Buffer &b)
{
    auto copy = closed_;
    for (auto &s : copy)
    {
        if (s.fn)
            s.fn(b);
    }
}
