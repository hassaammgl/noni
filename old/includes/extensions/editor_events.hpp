#pragma once

#include <editor/buffer.hpp>
#include <editor/undo.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Minimal multicast editor events (UI-independent). Subscriptions are owner-tagged.
class EditorEvents
{
public:
    using BufferFn = std::function<void(Buffer &)>;
    using BufferChangeFn = std::function<void(Buffer &, const TextChange &)>;

    template <typename Fn>
    struct Sub
    {
        std::uint64_t id = 0;
        std::string owner;
        Fn fn;
    };

    std::uint64_t on_buffer_opened(BufferFn fn, std::string owner);
    std::uint64_t on_buffer_changed(BufferChangeFn fn, std::string owner);
    std::uint64_t on_buffer_saved(BufferFn fn, std::string owner);
    std::uint64_t on_buffer_closed(BufferFn fn, std::string owner);

    void unsubscribe(std::uint64_t id);
    void unsubscribe_owner(const std::string &owner);

    void emit_buffer_opened(Buffer &b);
    void emit_buffer_changed(Buffer &b, const TextChange &ch);
    void emit_buffer_saved(Buffer &b);
    void emit_buffer_closed(Buffer &b);

private:
    std::uint64_t next_id_ = 1;
    std::vector<Sub<BufferFn>> opened_;
    std::vector<Sub<BufferChangeFn>> changed_;
    std::vector<Sub<BufferFn>> saved_;
    std::vector<Sub<BufferFn>> closed_;
};
