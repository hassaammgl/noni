#include "editor_impl.hpp"

void Editor::handle_insert_input(int key)
{
    if (!tab)
        return;

    if (pending_j)
    {
        pending_j = false;
        if (key == 'j')
        {
            const int col = tab->cursor().column;
            if (col > 0)
            {
                const auto &row = tab->buffer().lines()[static_cast<std::size_t>(tab->cursor().line)];
                const int new_col = static_cast<int>(
                    TextMetrics::prev_cp(row, static_cast<std::size_t>(col)));
                tab->buffer().delete_char_before(tab->cursor().line, col);
                tab->cursor().column = new_col;
            }
            leave_insert_mode();
            clamp_cursor();
            update_scroll();
            return;
        }
    }

    switch (key)
    {
    case 27:
        leave_insert_mode();
        break;
    case KEY_UP:
        pending_j = false;
        split_insert_edit();
        move_cursor_up();
        break;
    case KEY_DOWN:
        pending_j = false;
        split_insert_edit();
        move_cursor_down();
        break;
    case KEY_LEFT:
        pending_j = false;
        split_insert_edit();
        move_cursor_left();
        break;
    case KEY_RIGHT:
        pending_j = false;
        split_insert_edit();
        move_cursor_right();
        break;
    case KEY_PPAGE:
        pending_j = false;
        split_insert_edit();
        page_up();
        break;
    case KEY_NPAGE:
        pending_j = false;
        split_insert_edit();
        page_down();
        break;
    case KEY_BACKSPACE:
    case 127:
    case 8:
        pending_j = false;
        if (tab->cursor().column > 0)
        {
            const auto &row = tab->buffer().lines()[static_cast<std::size_t>(tab->cursor().line)];
            const int col = tab->cursor().column;
            const int new_col = static_cast<int>(
                TextMetrics::prev_cp(row, static_cast<std::size_t>(col)));
            tab->buffer().delete_char_before(tab->cursor().line, col);
            tab->cursor().column = new_col;
        }
        else if (tab->cursor().line > 0)
        {
            tab->buffer().delete_char_before(tab->cursor().line, tab->cursor().column);
            tab->cursor().line--;
            tab->cursor().column = static_cast<int>(tab->buffer().lines()[tab->cursor().line].size());
        }
        break;
    case KEY_DC:
        pending_j = false;
        tab->buffer().delete_char_at(tab->cursor().line, tab->cursor().column);
        break;
    case '\t':
        pending_j = false;
        tab->buffer().insert_char(tab->cursor().line, tab->cursor().column, '\t');
        tab->cursor().column++;
        break;
    case '\n':
    case KEY_ENTER:
        pending_j = false;
        tab->buffer().insert_newline(tab->cursor().line, tab->cursor().column);
        tab->cursor().line++;
        tab->cursor().column = 0;
        break;
#ifdef KEY_SIC
    case KEY_SIC:
        pending_j = false;
        paste_clipboard();
        break;
#endif
    default:
        if (key >= 32 && key <= 126)
        {
            tab->buffer().insert_char(tab->cursor().line, tab->cursor().column, static_cast<char>(key));
            tab->cursor().column++;
            pending_j = (key == 'j');
        }
        else
        {
            pending_j = false;
        }
        break;
    }

    clamp_cursor();
    sync_preferred_display(tab->active_window());
    update_scroll();
}

void Editor::insert_utf8(std::string_view utf8)
{
    if (!tab || tab->mode != EditorMode::Insert || utf8.empty())
        return;
    pending_j = false;
    auto [line, col] = tab->buffer().insert_text(tab->cursor().line, tab->cursor().column, utf8);
    tab->cursor().line = line;
    tab->cursor().column = col;
    clamp_cursor();
    sync_preferred_display(tab->active_window());
    update_scroll();
}
