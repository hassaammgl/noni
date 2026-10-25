// P2.2 — see editor_state.hpp.
#include "core/editor_state.hpp"

namespace noni {

EditorState::EditorState() {
    buffers.emplace_back();
    WindowState w;
    w.buffer = 0;
    windows.push_back(w);
}

eresult<BufferId> EditorState::open_file(const std::filesystem::path& p) {
    auto loaded = Buffer::load(p);
    if (!loaded) {
        return std::unexpected(loaded.error());
    }
    buffers.push_back(std::move(*loaded));
    return static_cast<BufferId>(buffers.size() - 1);
}

Buffer* EditorState::buffer(BufferId id) {
    if (id >= buffers.size()) {
        return nullptr;
    }
    return &buffers[static_cast<std::size_t>(id)];
}

const Buffer* EditorState::buffer(BufferId id) const {
    if (id >= buffers.size()) {
        return nullptr;
    }
    return &buffers[static_cast<std::size_t>(id)];
}

BufferId EditorState::buffer_id(const Buffer& b) const {
    for (std::size_t i = 0; i < buffers.size(); ++i) {
        if (&buffers[i] == &b) {
            return static_cast<BufferId>(i);
        }
    }
    return k_no_buffer;
}

} // namespace noni
