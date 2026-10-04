#include "editor_impl.hpp"
#include <utils/clipboard.hpp>
#include <utils/messages.hpp>

bool Editor::yank_range(const TextRange &range)
{
    if (!tab || range.empty())
        return false;

    std::string text = tab->buffer().get_range_text(
        range.start.line, range.start.column, range.end.line, range.end.column);

    RegisterValue value;
    value.text = text;
    value.type = (range.kind == SelectionKind::Line) ? RegisterType::Line : RegisterType::Character;
    registers().yank_to_pending(std::move(value));

    if (clipboard_sync_ && !text.empty())
    {
        std::string hint;
        (void)Clipboard::write(text, &hint);
        if (!hint.empty())
            Messages::warning(hint);
    }
    return true;
}
