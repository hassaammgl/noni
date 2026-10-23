#include <components/lsp_picker.hpp>
#include <ui/theme.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <cctype>
#include <format>

void LspPicker::handle_input(int key)
{
    if (!active_)
        return;
    switch (key)
    {
    case KEY_UP:
        if (!filtered_.empty())
            selected_ = (selected_ + static_cast<int>(filtered_.size()) - 1) %
                        static_cast<int>(filtered_.size());
        ensure_selection_visible();
        return;
    case KEY_DOWN:
        if (!filtered_.empty())
            selected_ = (selected_ + 1) % static_cast<int>(filtered_.size());
        ensure_selection_visible();
        return;
    case KEY_PPAGE:
        selected_ = std::max(0, selected_ - std::max(1, height - 3));
        ensure_selection_visible();
        return;
    case KEY_NPAGE:
        selected_ = std::min(
            static_cast<int>(filtered_.size()) - 1,
            selected_ + std::max(1, height - 3));
        if (selected_ < 0)
            selected_ = 0;
        ensure_selection_visible();
        return;
    case KEY_BACKSPACE:
    case 127:
    case 8:
        if (!query_.empty())
        {
            TextMetrics::pop_codepoint(query_);
            refilter();
        }
        return;
    default:
        if (key >= 32 && key <= 126)
        {
            query_.push_back(static_cast<char>(key));
            refilter();
        }
        return;
    }
}

void LspPicker::insert_utf8(std::string_view utf8)
{
    if (!active_ || utf8.empty())
        return;
    query_.append(utf8.data(), utf8.size());
    refilter();
}

bool LspPicker::take_location(LspLocation &out)
{
    if (!active_ || filtered_.empty())
        return false;
    const int idx = filtered_[static_cast<std::size_t>(selected_)];
    const auto *p = std::get_if<LspLocation>(&items_[static_cast<std::size_t>(idx)]);
    if (!p)
        return false;
    out = *p;
    close();
    return true;
}

bool LspPicker::take_symbol(LspSymbol &out)
{
    if (!active_ || filtered_.empty())
        return false;
    const int idx = filtered_[static_cast<std::size_t>(selected_)];
    const auto *p = std::get_if<LspSymbol>(&items_[static_cast<std::size_t>(idx)]);
    if (!p)
        return false;
    out = *p;
    close();
    return true;
}

bool LspPicker::take_action(LspCodeAction &out)
{
    if (!active_ || filtered_.empty())
        return false;
    const int idx = filtered_[static_cast<std::size_t>(selected_)];
    const auto *p = std::get_if<LspCodeAction>(&items_[static_cast<std::size_t>(idx)]);
    if (!p)
        return false;
    out = *p;
    close();
    return true;
}
