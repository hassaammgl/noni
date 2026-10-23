#include <components/messages_panel.hpp>
#include <ui/theme.hpp>
#include <utils/messages.hpp>

#include <algorithm>

void MessagesPanel::draw()
{
    if (!window || !active)
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Popup));

    // Full clear with visible text attributes.
    wattron(window, COLOR_PAIR(Theme::Popup));
    for (int r = 0; r < height; ++r)
        mvwhline(window, r, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::Popup));

    wattron(window, COLOR_PAIR(Theme::PopupBorder));
    box(window, 0, 0);
    std::string hdr = title_ + "  |  Esc=close  j/k or arrows=scroll";
    if (static_cast<int>(hdr.size()) > width - 4)
        hdr.resize(static_cast<std::size_t>(std::max(0, width - 4)));
    mvwprintw(window, 0, 2, " %s ", hdr.c_str());
    wattroff(window, COLOR_PAIR(Theme::PopupBorder));

    const auto &entries = Messages::all();
    const int body_h = std::max(0, height - 2);
    int row = 1;

    wattron(window, COLOR_PAIR(Theme::Popup));
    for (std::size_t i = static_cast<std::size_t>(scroll_y);
         i < entries.size() && row <= body_h;
         ++i, ++row)
    {
        std::string text = entries[i];
        // Byte-safe ASCII truncation (help docs are ASCII-only).
        const int max_w = std::max(0, width - 4);
        if (static_cast<int>(text.size()) > max_w)
            text.resize(static_cast<std::size_t>(max_w));
        mvwprintw(window, row, 2, "%s", text.c_str());
    }
    wattroff(window, COLOR_PAIR(Theme::Popup));

    // Scroll hint in bottom border.
    if (!entries.empty() && height >= 2)
    {
        const int max_scroll = std::max(0, static_cast<int>(entries.size()) - body_h);
        wattron(window, COLOR_PAIR(Theme::PopupBorder));
        if (max_scroll > 0)
            mvwprintw(window, height - 1, 2, " %d/%d ", scroll_y + 1, max_scroll + 1);
        wattroff(window, COLOR_PAIR(Theme::PopupBorder));
    }
}

void MessagesPanel::open(std::string title)
{
    active = true;
    scroll_y = 0;
    title_ = title.empty() ? "Messages" : std::move(title);
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

    const int body_h = std::max(1, height - 2);
    const int max_scroll = std::max(0, static_cast<int>(entries.size()) - body_h);

    if (key == KEY_UP || key == 'k')
        scroll_y = std::max(0, scroll_y - 1);

    if (key == KEY_DOWN || key == 'j')
        scroll_y = std::min(max_scroll, scroll_y + 1);

    if (key == KEY_PPAGE)
        scroll_y = std::max(0, scroll_y - body_h);

    if (key == KEY_NPAGE)
        scroll_y = std::min(max_scroll, scroll_y + body_h);

    if (key == 'g')
        scroll_y = 0;

    if (key == 'G')
        scroll_y = max_scroll;
}
