#include <editor/editor_tab.hpp>

std::string EditorTab::display_name() const
{
    const fs::path path = buffer.get_buffer_path();
    if (path.empty())
        return "[No Name]";
    return path.filename().string();
}

bool EditorTab::is_untitled() const
{
    return buffer.get_buffer_path().empty();
}
