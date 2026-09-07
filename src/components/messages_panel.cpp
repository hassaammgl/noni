#include <components/messages_panel.hpp>
#include <ui/theme.hpp>
#include <utils/messages.hpp>

void MessagesPanel::draw()
{
    if (!window || !active)
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Popup));

    mvwprintw(window, 0, 1, "Messages (Esc to close)");

    const auto &entries = Messages::all();
    int row = 1;

    for (std::size_t i = static_cast<std::size_t>(scroll_y);
         i < entries.size() && row < height;
         ++i, ++row)
    {
        mvwprintw(window, row, 1, "%s", entries[i].c_str());
    }
}

void MessagesPanel::open()
{
    active = true;
    scroll_y = 0;
}

void MessagesPanel::close()
{
    active = false;
    scroll_y = 0;
}

bool MessagesPanel::is_active() const
{
    return active;
}

void MessagesPanel::handle_input(int key)
{
    if (key == 27)
    {
        close();
        return;
    }

    const auto &entries = Messages::all();
    if (entries.empty())
        return;

    if (key == KEY_UP && scroll_y > 0)
        scroll_y--;

    if (key == KEY_DOWN &&
        scroll_y + 1 < static_cast<int>(entries.size()))
        scroll_y++;
}
