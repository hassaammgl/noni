#include <components/search_panel.hpp>
#include <ui/theme.hpp>
#include <utils/str.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <format>
#include <string_view>


void SearchPanel::set_focused(bool v)
{
    focused = v;
}

void SearchPanel::open()
{
    focused = true;
    field = SearchField::Query;
}

void SearchPanel::close()
{
    focused = false;
}

bool SearchPanel::poll()
{
    const auto before = seen_version;
    sync_results();
    return seen_version != before || engine.is_searching();
}

SearchPanelAction SearchPanel::handle_input(int key)
{
    sync_results();

    if (key == KEY_F(6))
    {
        opts.match_case = !opts.match_case;
        if (!query.empty())
            run_search();
        return SearchPanelAction::None;
    }
    if (key == KEY_F(7))
    {
        opts.whole_word = !opts.whole_word;
        if (!query.empty())
            run_search();
        return SearchPanelAction::None;
    }
    if (key == KEY_F(8))
    {
        opts.use_regex = !opts.use_regex;
        if (!query.empty())
            run_search();
        return SearchPanelAction::None;
    }

    if (key == '\t')
    {
        if (field == SearchField::Query)
            field = SearchField::Replace;
        else if (field == SearchField::Replace)
            field = SearchField::Results;
        else
            field = SearchField::Query;
        return SearchPanelAction::None;
    }

    if (key == KEY_BTAB)
    {
        if (field == SearchField::Query)
            field = SearchField::Results;
        else if (field == SearchField::Replace)
            field = SearchField::Query;
        else
            field = SearchField::Replace;
        return SearchPanelAction::None;
    }

    if (field == SearchField::Query || field == SearchField::Replace)
    {
        std::string &target = (field == SearchField::Query) ? query : replace_text;

        if (key == '\n' || key == '\r' || key == KEY_ENTER)
        {
            if (field == SearchField::Query)
            {
                run_search();
                field = SearchField::Results;
            }
            else
            {
                return SearchPanelAction::ReplaceAll;
            }
            return SearchPanelAction::None;
        }
        if (key == 27)
            return SearchPanelAction::FocusEditor;
        if (key == KEY_BACKSPACE || key == 127 || key == 8)
        {
            if (!target.empty())
                TextMetrics::pop_codepoint(target);
            if (field == SearchField::Query)
                run_search(); // live search
            return SearchPanelAction::None;
        }
        if (key >= 32 && key <= 126)
        {
            target.push_back(static_cast<char>(key));
            if (field == SearchField::Query)
                run_search();
            return SearchPanelAction::None;
        }
        return SearchPanelAction::None;
    }

    // Results navigation
    const int lh = list_height();
    switch (key)
    {
    case 27:
        return SearchPanelAction::FocusEditor;
    case 'q':
        return SearchPanelAction::FocusEditor;
    case KEY_UP:
    case 'k':
        if (selected > 0)
            --selected;
        break;
    case KEY_DOWN:
    case 'j':
        if (selected + 1 < static_cast<int>(cached.size()))
            ++selected;
        break;
    case KEY_PPAGE:
        selected = std::max(0, selected - std::max(1, lh));
        break;
    case KEY_NPAGE:
        selected = std::min(
            static_cast<int>(cached.size()) - 1,
            selected + std::max(1, lh));
        if (selected < 0)
            selected = 0;
        break;
    case KEY_HOME:
    case 'g':
        selected = 0;
        break;
    case KEY_END:
    case 'G':
        selected = cached.empty() ? 0 : static_cast<int>(cached.size()) - 1;
        break;
    case '\n':
    case '\r':
    case KEY_ENTER:
    case 'o':
        if (!cached.empty())
            return SearchPanelAction::OpenMatch;
        break;
    case 'r':
        return SearchPanelAction::ReplaceAll;
    default:
        break;
    }

    ensure_selection_visible(lh);
    return SearchPanelAction::None;
}

