#include <components/search_panel.hpp>
#include <ui/theme.hpp>
#include <utils/str.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <format>
#include <string_view>


void SearchPanel::insert_utf8(std::string_view utf8)
{
    if (!in_text_field() || utf8.empty())
        return;
    std::string &target = (field == SearchField::Query) ? query : replace_text;
    target.append(utf8.data(), utf8.size());
    if (field == SearchField::Query)
        run_search();
}

TextMatch SearchPanel::selected_match() const
{
    if (cached.empty() || selected < 0 || selected >= static_cast<int>(cached.size()))
        return {};
    return cached[static_cast<std::size_t>(selected)];
}

std::vector<TextMatch> SearchPanel::results() const
{
    return cached;
}

const std::string &SearchPanel::get_query() const
{
    return query;
}

const std::string &SearchPanel::get_replace() const
{
    return replace_text;
}

TextSearchOptions SearchPanel::get_options() const
{
    return opts;
}
