#include "editor_impl.hpp"

bool Editor::yank_range(const TextRange &range)
{
    if (!tab || range.empty())
        return false;

    std::string text = tab->buffer().get_range_text(
        range.start.line, range.start.column, range.end.line, range.end.column);

    // Linewise yank should end with newline when not through-EOF extract already.
    if (range.kind == SelectionKind::Line && !text.empty() && text.back() != '\n')
    {
        // Through-EOF extracts lack trailing newline; paste linewise still works.
    }

    RegisterValue value;
    value.text = std::move(text);
    value.type = (range.kind == SelectionKind::Line) ? RegisterType::Line : RegisterType::Character;
    registers().yank_to_pending(std::move(value));
    return true;
}

bool Editor::delete_range(const TextRange &range)
{
    if (!tab || range.empty())
        return false;

    yank_range(range);
    begin_buffer_edit();
    tab->buffer().delete_range(
        range.start.line, range.start.column, range.end.line, range.end.column);
    tab->cursor() = range.start;
    clamp_cursor();
    sync_preferred_display(tab->active_window());
    return true;
}

bool Editor::execute_operator(PendingOperator op, const TextRange &range, bool enter_insert)
{
    if (!tab || op == PendingOperator::None)
        return false;

    if (op == PendingOperator::Yank)
    {
        yank_range(range);
        return true;
    }

    if (op == PendingOperator::Delete || op == PendingOperator::Change)
    {
        if (!delete_range(range))
            return false;
        if (op == PendingOperator::Change || enter_insert)
            enter_insert_mode(); // keeps open edit transaction from delete_range
        else
            end_buffer_edit();
        return true;
    }
    return false;
}

bool Editor::operator_on_visual(PendingOperator op, bool enter_insert)
{
    if (!tab || !tab->active_window().has_selection())
        return false;

    TextRange range = tab->active_window().selected_range();
    leave_visual();
    return execute_operator(op, range, enter_insert);
}

TextRange Editor::visual_or_motion_range(const MotionResult &motion) const
{
    if (in_visual())
        return tab->active_window().selected_range();

    TextRange range = motion.range;
    if (motion.linewise)
        range.kind = SelectionKind::Line;
    else
        range.kind = SelectionKind::Character;
    return range;
}

bool Editor::paste_register(bool after)
{
    if (!tab)
        return false;

    const char reg = registers().pending();
    const RegisterValue value = registers().get(reg);
    registers().clear_pending();
    if (value.empty())
        return false;

    clamp_cursor();
    const bool nested = tab->buffer().is_edit_active();
    if (!nested)
        begin_buffer_edit();

    if (value.type == RegisterType::Line)
    {
        std::string text = value.text;
        if (!text.empty() && text.back() == '\n')
            text.pop_back();

        if (after)
        {
            const int line = tab->cursor().line;
            const int col = static_cast<int>(tab->buffer().lines()[line].size());
            tab->buffer().insert_newline(line, col);
            tab->cursor().line = line + 1;
            tab->cursor().column = 0;
        }
        else
        {
            tab->buffer().insert_empty_line(tab->cursor().line);
            tab->cursor().column = 0;
        }

        const auto [el, ec] = tab->buffer().insert_text(
            tab->cursor().line, tab->cursor().column, text);
        (void)ec;
        tab->cursor().line = el;
        tab->cursor().column = 0;
    }
    else
    {
        Cursor c = tab->cursor();
        if (after)
        {
            const auto &lines = tab->buffer().lines();
            if (!lines.empty())
            {
                const auto &row = lines[static_cast<std::size_t>(c.line)];
                if (static_cast<std::size_t>(c.column) < row.size())
                    c.column = static_cast<int>(
                        TextMetrics::next_cp(row, static_cast<std::size_t>(c.column)));
            }
        }
        const auto [el, ec] = tab->buffer().insert_text(c.line, c.column, value.text);
        tab->cursor().line = el;
        tab->cursor().column = ec;
    }

    if (!nested)
        end_buffer_edit();

    clamp_cursor();
    sync_preferred_display(tab->active_window());
    update_scroll();
    return true;
}
