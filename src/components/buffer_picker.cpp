#include <components/buffer_picker.hpp>
#include <ui/theme.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <cctype>
#include <format>

namespace
{
    std::string lower(std::string s)
    {
        for (char &c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }
}

void BufferPicker::refilter()
{
    matches_.clear();
    if (!buffers_ || !buffers_->has_tabs())
        return;

    const auto &tabs = buffers_->get_tabs();
    const std::string q = lower(query_);

    for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
    {
        const auto &tab = tabs[static_cast<std::size_t>(i)];
        std::string name = tab.display_name();
        if (tab.buffer().is_dirty())
            name += " [+]";

        int score = 0;
        if (q.empty())
        {
            score = 1000 - i; // keep open order when no query
        }
        else
        {
            score = Fuzzy::score(lower(name), q);
            if (score < 0)
            {
                const auto path = tab.buffer().get_buffer_path().string();
                score = Fuzzy::score(lower(path), q);
            }
            if (score < 0)
                continue;
        }

        FuzzyMatch m;
        m.path = fs::path(std::to_string(i)); // tab index encoded for take_selection
        m.display = std::format("{}: {}", i + 1, name);
        m.score = score;
        matches_.push_back(std::move(m));
    }

    if (!q.empty())
    {
        std::stable_sort(matches_.begin(), matches_.end(), [](const FuzzyMatch &a, const FuzzyMatch &b) {
            return a.score > b.score;
        });
    }

    if (selected_ >= static_cast<int>(matches_.size()))
        selected_ = matches_.empty() ? 0 : static_cast<int>(matches_.size()) - 1;
    if (selected_ < 0)
        selected_ = 0;
    ensure_selection_visible();
}

void BufferPicker::ensure_selection_visible()
{
    const int list_height = std::max(1, height - 3);
    if (selected_ < scroll_y_)
        scroll_y_ = selected_;
    if (selected_ >= scroll_y_ + list_height)
        scroll_y_ = selected_ - list_height + 1;
    if (scroll_y_ < 0)
        scroll_y_ = 0;
}

void BufferPicker::draw()
{
    if (!window || !active_)
        return;

    werase(window);
    leaveok(window, FALSE);
    wbkgd(window, COLOR_PAIR(Theme::Popup));
    wattron(window, COLOR_PAIR(Theme::Popup));
    for (int row = 0; row < height; ++row)
        mvwhline(window, row, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::Popup));

    wattron(window, COLOR_PAIR(Theme::PopupBorder));
    mvwhline(window, 0, 0, ACS_HLINE, width);
    mvwhline(window, height - 2, 0, ACS_HLINE, width);
    wattroff(window, COLOR_PAIR(Theme::PopupBorder));

    wattron(window, COLOR_PAIR(Theme::Popup));
    mvwprintw(window, 0, 2, " buffers  %zu ", matches_.size());
    wattroff(window, COLOR_PAIR(Theme::Popup));

    const int list_height = std::max(1, height - 3);
    for (int row = 0; row < list_height; ++row)
    {
        const int idx = scroll_y_ + row;
        if (idx >= static_cast<int>(matches_.size()))
            break;
        const bool is_sel = (idx == selected_);
        const short pair = is_sel ? Theme::PopupSelected : Theme::Popup;
        wattron(window, COLOR_PAIR(pair));
        if (is_sel)
            wattron(window, A_BOLD);
        mvwhline(window, row + 1, 0, ' ', width);
        const auto &m = matches_[static_cast<std::size_t>(idx)];
        mvwprintw(window, row + 1, 2, "%.*s", width - 3, m.display.c_str());
        if (is_sel)
            wattroff(window, A_BOLD);
        wattroff(window, COLOR_PAIR(pair));
    }

    wattron(window, COLOR_PAIR(Theme::InputFocus));
    mvwhline(window, height - 1, 0, ' ', width);
    mvwprintw(window, height - 1, 0, "> %s", query_.c_str());
    wattroff(window, COLOR_PAIR(Theme::InputFocus));
    wmove(window, height - 1, 2 + static_cast<int>(query_.size()));
}

void BufferPicker::open()
{
    active_ = true;
    query_.clear();
    selected_ = 0;
    scroll_y_ = 0;
    refilter();
    if (buffers_ && buffers_->has_tabs())
        selected_ = buffers_->get_active_index();
    ensure_selection_visible();
}

void BufferPicker::close()
{
    active_ = false;
    query_.clear();
    matches_.clear();
    selected_ = 0;
    scroll_y_ = 0;
}

