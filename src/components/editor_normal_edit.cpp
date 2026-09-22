#include "editor_impl.hpp"

void Editor::handle_normal_edits(int key)
{
    switch (key)
    {
    case 'o':
    {
        clamp_cursor();
        begin_buffer_edit();
        const int line = tab->cursor().line;
        const int col = static_cast<int>(tab->buffer().lines()[line].size());
        tab->buffer().insert_newline(line, col);
        tab->cursor().line = line + 1;
        tab->cursor().column = 0;
        enter_insert_mode();
        break;
    }
    case 'O':
        clamp_cursor();
        begin_buffer_edit();
        tab->buffer().insert_empty_line(tab->cursor().line);
        tab->cursor().column = 0;
        enter_insert_mode();
        break;
    case 'i':
        enter_insert_mode();
        break;
    case 'I':
        clamp_cursor();
        tab->cursor().column = 0;
        enter_insert_mode();
        break;
    case 'a':
        enter_insert_mode();
        if (!tab->buffer().lines().empty())
        {
            clamp_cursor();
            const auto &row = tab->buffer().lines()[tab->cursor().line];
            if (static_cast<std::size_t>(tab->cursor().column) < row.size())
                tab->cursor().column = static_cast<int>(
                    TextMetrics::next_cp(row, static_cast<std::size_t>(tab->cursor().column)));
        }
        break;
    case 'A':
        enter_insert_mode();
        if (!tab->buffer().lines().empty())
        {
            clamp_cursor();
            tab->cursor().column = static_cast<int>(tab->buffer().lines()[tab->cursor().line].size());
        }
        break;
    case 's':
        clamp_cursor();
        begin_buffer_edit();
        tab->buffer().delete_char_at(tab->cursor().line, tab->cursor().column);
        enter_insert_mode();
        break;
    case 'S':
    {
        clamp_cursor();
        TextRange range = normalize_line_range(
            tab->cursor(), tab->cursor(), static_cast<int>(tab->buffer().lines().size()));
        execute_operator(PendingOperator::Change, range, true);
        break;
    }
    case 'C':
    {
        clamp_cursor();
        MotionResult motion = Motion::line_end(win());
        TextRange range = motion.range;
        range.kind = SelectionKind::Character;
        execute_operator(PendingOperator::Change, range, true);
        break;
    }
    case 'x':
        clamp_cursor();
        begin_buffer_edit();
        {
            const auto &lines = tab->buffer().lines();
            if (!lines.empty())
            {
                const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
                if (tab->cursor().column < static_cast<int>(row.size()))
                {
                    const std::size_t start = static_cast<std::size_t>(tab->cursor().column);
                    const std::size_t end = TextMetrics::next_cp(row, start);
                    RegisterValue value;
                    value.text = row.substr(start, end - start);
                    value.type = RegisterType::Character;
                    registers().yank_to_pending(std::move(value));
                }
            }
        }
        tab->buffer().delete_char_at(tab->cursor().line, tab->cursor().column);
        end_buffer_edit();
        sync_preferred_display(tab->active_window());
        break;
    case 'p':
        paste_register(true);
        break;
    case 'P':
        paste_register(false);
        break;
#ifdef KEY_SIC
    case KEY_SIC:
        paste_clipboard();
        break;
#endif
    case 'u':
        undo();
        break;
    case 18: // Ctrl-R
        redo();
        break;
    case 15: // Ctrl-O jump back
    {
        const JumpEntry *entry = jumps().back(current_buffer_id(), tab->cursor());
        if (entry)
            jump_to(entry->buffer_id, entry->pos);
        break;
    }
    case 9: // Ctrl-I / Tab — jump forward (may conflict with insert; normal only)
    {
        const JumpEntry *entry = jumps().forward();
        if (entry)
            jump_to(entry->buffer_id, entry->pos);
        break;
    }
    default:
        break;
    }
}
