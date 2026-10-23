#include "editor_impl.hpp"

Buffer &Editor::get_buffer()
{
    static Buffer empty;
    return tab ? tab->buffer() : empty;
}

const Buffer &Editor::get_buffer() const
{
    static Buffer empty;
    return tab ? tab->buffer() : empty;
}
