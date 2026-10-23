#include <components/completion_picker.hpp>
#include <ui/theme.hpp>

#include <algorithm>

void CompletionPicker::ensure_selection_visible()
{
    if (selected_ < scroll_y_)
        scroll_y_ = selected_;
    const int visible = std::max(1, height - 2);
    if (selected_ >= scroll_y_ + visible)
        scroll_y_ = selected_ - visible + 1;
}

void CompletionPicker::draw()
{
    if (!window || !active_)
        return;

    werase(window);
    leaveok(window, TRUE);
    wbkgd(window, COLOR_PAIR(Theme::Popup));
    wattron(window, COLOR_PAIR(Theme::PopupBorder));
    box(window, 0, 0);
    wattroff(window, COLOR_PAIR(Theme::PopupBorder));

    wattron(window, COLOR_PAIR(Theme::Popup));
    mvwprintw(window, 0, 2, " Completions ");
    wattroff(window, COLOR_PAIR(Theme::Popup));

    const int visible = std::max(1, height - 2);
    for (int row = 0; row < visible; ++row)
    {
        const int idx = scroll_y_ + row;
        if (idx >= static_cast<int>(list_.items.size()))
            break;

        const auto &item = list_.items[static_cast<std::size_t>(idx)];
        const short pair = (idx == selected_) ? Theme::PopupSelected : Theme::Popup;
        wattron(window, COLOR_PAIR(pair));
        mvwhline(window, row + 1, 1, ' ', std::max(0, width - 2));
        std::string line = item.label;
        if (!item.detail.empty())
            line += "  " + item.detail;
        if (static_cast<int>(line.size()) > width - 3)
            line.resize(static_cast<std::size_t>(std::max(0, width - 3)));
        mvwprintw(window, row + 1, 2, "%s", line.c_str());
        wattroff(window, COLOR_PAIR(pair));
    }
}

void CompletionPicker::open(CompletionList list)
{
    list_ = std::move(list);
    selected_ = 0;
    scroll_y_ = 0;
    active_ = true;
}

void CompletionPicker::close()
{
    active_ = false;
    list_ = {};
    selected_ = 0;
    scroll_y_ = 0;
}

void CompletionPicker::handle_input(int key)
{
    if (!active_)
        return;
    const int n = static_cast<int>(list_.items.size());
    if (n <= 0)
        return;

    switch (key)
    {
    case KEY_UP:
        selected_ = (selected_ + n - 1) % n;
        break;
    case KEY_DOWN:
        selected_ = (selected_ + 1) % n;
        break;
    case KEY_PPAGE:
        selected_ = std::max(0, selected_ - std::max(1, height - 2));
        break;
    case KEY_NPAGE:
        selected_ = std::min(n - 1, selected_ + std::max(1, height - 2));
        break;
    default:
        break;
    }
    ensure_selection_visible();
}

bool CompletionPicker::take_selection(CompletionItem &out)
{
    if (!active_ || list_.items.empty())
        return false;
    if (selected_ < 0 || selected_ >= static_cast<int>(list_.items.size()))
        return false;
    out = list_.items[static_cast<std::size_t>(selected_)];
    close();
    return true;
}
