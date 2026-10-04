#include <components/hover_popup.hpp>
#include <ui/theme.hpp>
#include <utils/str.hpp>

#include <algorithm>

void HoverPopup::open(std::string title, std::string body)
{
    lines_.clear();
    if (!title.empty())
        lines_.push_back(std::move(title));
    for (auto &line : StrUtils::split(body, '\n'))
        lines_.push_back(std::move(line));
    if (lines_.empty())
        lines_.push_back("(empty hover)");
    scroll_ = 0;
    active_ = true;
}

void HoverPopup::close()
{
    active_ = false;
    lines_.clear();
    scroll_ = 0;
}

void HoverPopup::handle_input(int key)
{
    if (!active_)
        return;
    const int view = std::max(1, height - 2);
    if (key == KEY_UP || key == 'k')
        scroll_ = std::max(0, scroll_ - 1);
    else if (key == KEY_DOWN || key == 'j')
        scroll_ = std::min(std::max(0, static_cast<int>(lines_.size()) - view), scroll_ + 1);
    else if (key == KEY_PPAGE)
        scroll_ = std::max(0, scroll_ - view);
    else if (key == KEY_NPAGE)
        scroll_ = std::min(std::max(0, static_cast<int>(lines_.size()) - view), scroll_ + view);
    else if (key == 27 || key == 3 || key == 'q')
        close();
}

void HoverPopup::draw()
{
    if (!window || !active_)
        return;
    werase(window);
    wbkgd(window, COLOR_PAIR(Theme::Popup));
    box(window, 0, 0);
    const int view = std::max(1, height - 2);
    for (int i = 0; i < view; ++i)
    {
        const int idx = scroll_ + i;
        if (idx >= static_cast<int>(lines_.size()))
            break;
        const std::string &line = lines_[static_cast<std::size_t>(idx)];
        mvwprintw(window, i + 1, 1, "%.*s", width - 2, line.c_str());
    }
}
