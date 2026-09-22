#include "editor_impl.hpp"

std::uintptr_t Editor::current_buffer_id() const
{
    if (!tab || !tab->active_window().has_buffer())
        return 0;
    return reinterpret_cast<std::uintptr_t>(&tab->buffer());
}

void Editor::on_buffer_closed(std::uintptr_t buffer_id)
{
    marks().invalidate_buffer(buffer_id);
    jumps().invalidate_buffer(buffer_id);
}

void Editor::record_jump_from_here()
{
    if (!tab)
        return;
    jumps().push(current_buffer_id(), tab->cursor());
}

void Editor::jump_to(std::uintptr_t buffer_id, Cursor pos)
{
    if (!tab || buffer_id == 0)
        return;
    if (buffer_id != current_buffer_id())
    {
        // Cross-buffer jump: ask UI/BufferManager via core if available.
        if (!core_)
            return;
        bool found = false;
        const auto &tabs = core_->buffers().get_tabs();
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
        {
            for (Window *w : tabs[static_cast<std::size_t>(i)].layout.leaves())
            {
                if (w && w->has_buffer() &&
                    reinterpret_cast<std::uintptr_t>(&w->buffer()) == buffer_id)
                {
                    core_->buffers().switch_to(i);
                    tab = &core_->buffers().active();
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
        if (!found)
            return;
    }
    tab->cursor() = pos;
    clamp_cursor();
    update_scroll();
}
