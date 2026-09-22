#include <components/lsp_picker.hpp>
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

std::string LspPicker::label_for(const Payload &p) const
{
    if (const auto *loc = std::get_if<LspLocation>(&p))
        return loc->display.empty() ? loc->uri : loc->display;
    if (const auto *sym = std::get_if<LspSymbol>(&p))
    {
        if (!sym->detail.empty())
            return std::format("{} — {}", sym->name, sym->detail);
        return sym->name.empty() ? sym->location.display : sym->name;
    }
    if (const auto *act = std::get_if<LspCodeAction>(&p))
        return act->title;
    return {};
}

void LspPicker::refilter()
{
    filtered_.clear();
    const std::string q = lower(query_);
    for (int i = 0; i < static_cast<int>(items_.size()); ++i)
    {
        if (q.empty())
        {
            filtered_.push_back(i);
            continue;
        }
        const int score = Fuzzy::score(lower(label_for(items_[static_cast<std::size_t>(i)])), q);
        if (score >= 0)
            filtered_.push_back(i);
    }
    if (selected_ >= static_cast<int>(filtered_.size()))
        selected_ = filtered_.empty() ? 0 : static_cast<int>(filtered_.size()) - 1;
    if (selected_ < 0)
        selected_ = 0;
    ensure_selection_visible();
}

void LspPicker::ensure_selection_visible()
{
    const int list_height = std::max(1, height - 3);
    if (selected_ < scroll_y_)
        scroll_y_ = selected_;
    if (selected_ >= scroll_y_ + list_height)
        scroll_y_ = selected_ - list_height + 1;
    if (scroll_y_ < 0)
        scroll_y_ = 0;
}

void LspPicker::draw()
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
    mvwprintw(window, 0, 2, " %s  %zu ", title_.c_str(), filtered_.size());
    wattroff(window, COLOR_PAIR(Theme::Popup));

    const int list_height = std::max(1, height - 3);
    for (int row = 0; row < list_height; ++row)
    {
        const int fidx = scroll_y_ + row;
        if (fidx >= static_cast<int>(filtered_.size()))
            break;
        const int idx = filtered_[static_cast<std::size_t>(fidx)];
        const bool is_sel = (fidx == selected_);
        const short pair = is_sel ? Theme::PopupSelected : Theme::Popup;
        wattron(window, COLOR_PAIR(pair));
        if (is_sel)
            wattron(window, A_BOLD);
        mvwhline(window, row + 1, 0, ' ', width);
        const std::string label = label_for(items_[static_cast<std::size_t>(idx)]);
        mvwprintw(window, row + 1, 2, "%.*s", width - 3, label.c_str());
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

void LspPicker::open_locations(std::string title, std::vector<LspLocation> locs)
{
    active_ = true;
    title_ = std::move(title);
    query_.clear();
    selected_ = 0;
    scroll_y_ = 0;
    items_.clear();
    items_.reserve(locs.size());
    for (auto &l : locs)
        items_.push_back(std::move(l));
    refilter();
}

void LspPicker::open_symbols(std::string title, std::vector<LspSymbol> syms)
{
    active_ = true;
    title_ = std::move(title);
    query_.clear();
    selected_ = 0;
    scroll_y_ = 0;
    items_.clear();
    items_.reserve(syms.size());
    for (auto &s : syms)
        items_.push_back(std::move(s));
    refilter();
}

void LspPicker::open_actions(std::string title, std::vector<LspCodeAction> actions)
{
    active_ = true;
    title_ = std::move(title);
    query_.clear();
    selected_ = 0;
    scroll_y_ = 0;
    items_.clear();
    items_.reserve(actions.size());
    for (auto &a : actions)
        items_.push_back(std::move(a));
    refilter();
}

void LspPicker::close()
{
    active_ = false;
    query_.clear();
    items_.clear();
    filtered_.clear();
    selected_ = 0;
    scroll_y_ = 0;
}

