#include <components/editor.hpp>
#include <syntax/syntax.hpp>
#include <ui/theme.hpp>

#include <algorithm>

void Editor::bind(EditorTab *new_tab)
{
    tab = new_tab;
    pending_j = false;
}

void Editor::move_cursor_up()
{
    if (!tab)
        return;
    if (tab->cursor.line > 0)
        tab->cursor.line--;
    clamp_cursor();
}

void Editor::move_cursor_down()
{
    if (!tab)
        return;
    tab->cursor.line++;
    clamp_cursor();
}

void Editor::move_cursor_left()
{
    if (!tab)
        return;

    if (tab->cursor.column > 0)
    {
        tab->cursor.column--;
        return;
    }
    if (tab->cursor.line > 0)
    {
        tab->cursor.line--;
        const auto &lines = tab->buffer.lines();
        if (!lines.empty())
            tab->cursor.column = static_cast<int>(lines[tab->cursor.line].size());
    }
}

void Editor::move_cursor_right()
{
    if (!tab)
        return;

    const auto &lines = tab->buffer.lines();
    if (lines.empty())
        return;
    clamp_cursor();
    int max_column = static_cast<int>(lines[tab->cursor.line].size());
    if (tab->cursor.column < max_column)
    {
        tab->cursor.column++;
        return;
    }

    if (tab->cursor.line + 1 < static_cast<int>(lines.size()))
    {
        tab->cursor.line++;
        tab->cursor.column = 0;
    }
}

int Editor::page_step() const
{
    return std::max(1, height > 1 ? height - 1 : 1);
}

void Editor::page_up()
{
    if (!tab)
        return;
    clamp_cursor();
    const int step = page_step();
    tab->cursor.line = std::max(0, tab->cursor.line - step);
    tab->scroll_y = std::max(0, tab->scroll_y - step);
}

void Editor::page_down()
{
    if (!tab)
        return;
    clamp_cursor();
    const auto &lines = tab->buffer.lines();
    if (lines.empty())
        return;
    const int step = page_step();
    const int last = static_cast<int>(lines.size()) - 1;
    tab->cursor.line = std::min(last, tab->cursor.line + step);
    tab->scroll_y = std::min(
        std::max(0, last - std::max(0, height - 1)),
        tab->scroll_y + step);
}

void Editor::half_page_up()
{
    if (!tab)
        return;
    clamp_cursor();
    const int step = std::max(1, page_step() / 2);
    tab->cursor.line = std::max(0, tab->cursor.line - step);
    tab->scroll_y = std::max(0, tab->scroll_y - step);
}

void Editor::half_page_down()
{
    if (!tab)
        return;
    clamp_cursor();
    const auto &lines = tab->buffer.lines();
    if (lines.empty())
        return;
    const int step = std::max(1, page_step() / 2);
    const int last = static_cast<int>(lines.size()) - 1;
    tab->cursor.line = std::min(last, tab->cursor.line + step);
    tab->scroll_y = std::min(
        std::max(0, last - std::max(0, height - 1)),
        tab->scroll_y + step);
}

void Editor::update_scroll()
{
    if (!tab || height <= 0)
        return;

    if (tab->cursor.line < tab->scroll_y)
        tab->scroll_y = tab->cursor.line;

    if (tab->cursor.line >= tab->scroll_y + height)
        tab->scroll_y = tab->cursor.line - height + 1;

    if (tab->scroll_y < 0)
        tab->scroll_y = 0;

    if (width <= 0)
        return;

    if (tab->cursor.column < tab->scroll_x)
        tab->scroll_x = tab->cursor.column;

    if (tab->cursor.column >= tab->scroll_x + width)
        tab->scroll_x = tab->cursor.column - width + 1;

    if (tab->scroll_x < 0)
        tab->scroll_x = 0;
}

void Editor::clamp_cursor()
{
    if (!tab)
        return;

    const auto &lines = tab->buffer.lines();
    if (lines.empty())
    {
        tab->cursor.line = 0;
        tab->cursor.column = 0;
        return;
    }

    if (tab->cursor.line < 0)
        tab->cursor.line = 0;
    if (tab->cursor.line >= static_cast<int>(lines.size()))
        tab->cursor.line = static_cast<int>(lines.size()) - 1;

    int max_column = static_cast<int>(lines[tab->cursor.line].size());
    if (tab->cursor.column < 0)
        tab->cursor.column = 0;
    if (tab->cursor.column > max_column)
        tab->cursor.column = max_column;
}

void Editor::draw()
{
    if (!window || !tab)
        return;

    werase(window);
    leaveok(window, FALSE);
    wbkgd(window, COLOR_PAIR(Theme::Editor));
    wattron(window, COLOR_PAIR(Theme::Editor));
    for (int row = 0; row < height; ++row)
        mvwhline(window, row, 0, ' ', width);
    wattroff(window, COLOR_PAIR(Theme::Editor));

    clamp_cursor();
    update_scroll();

    const auto &content = tab->buffer.lines();
    const Language lang = Syntax::detect_language(tab->buffer.get_buffer_path());

    // Carry block-comment / string state from file start → first visible line.
    HighlightState state;
    const int first = tab->scroll_y;
    for (int i = 0; i < first && i < static_cast<int>(content.size()); ++i)
        (void)Syntax::highlight_line(content[static_cast<std::size_t>(i)], lang, state);

    for (int row = 0; row < height; ++row)
    {
        const int line_index = tab->scroll_y + row;
        if (line_index >= static_cast<int>(content.size()))
            continue;

        const std::string &line = content[static_cast<std::size_t>(line_index)];
        const auto tokens = Syntax::highlight_line(line, lang, state);

        const int sx = tab->scroll_x;
        const int line_len = static_cast<int>(line.size());

        // Paint gaps as editor bg, tokens with their colors (viewport clipped).
        for (const auto &tok : tokens)
        {
            const int tok_start = tok.start;
            const int tok_end = tok.start + tok.length;
            const int vis_start = std::max(tok_start, sx);
            const int vis_end = std::min(tok_end, sx + width);
            if (vis_start >= vis_end)
                continue;

            const short pair = Syntax::theme_pair(tok.kind);
            wattron(window, COLOR_PAIR(pair));
            mvwaddnstr(
                window,
                row,
                vis_start - sx,
                line.c_str() + vis_start,
                vis_end - vis_start);
            wattroff(window, COLOR_PAIR(pair));
        }

        // If no tokens covered the line (empty), still fine.
        (void)line_len;
    }

    wmove(window, tab->cursor.line - tab->scroll_y, tab->cursor.column - tab->scroll_x);
}

EditorMode Editor::get_mode() const
{
    return tab ? tab->mode : EditorMode::Normal;
}

std::string Editor::get_mode_label() const
{
    return get_mode() == EditorMode::Insert ? "INSERT" : "NORMAL";
}

void Editor::enter_insert_mode()
{
    if (!tab)
        return;
    pending_j = false;
    tab->mode = EditorMode::Insert;
}

void Editor::leave_insert_mode()
{
    if (!tab)
        return;

    pending_j = false;
    if (tab->mode != EditorMode::Insert)
        return;

    // Vim: after Esc, cursor sits on last inserted char (not after it).
    if (tab->cursor.column > 0)
        tab->cursor.column--;
    tab->mode = EditorMode::Normal;
}

void Editor::enter_normal_mode()
{
    if (!tab)
        return;

    if (tab->mode == EditorMode::Insert)
        leave_insert_mode();
    else
    {
        pending_j = false;
        tab->mode = EditorMode::Normal;
    }
}

void Editor::handle_normal_input(int key)
{
    if (!tab)
        return;

    switch (key)
    {
    case KEY_UP:
        move_cursor_up();
        break;
    case KEY_DOWN:
        move_cursor_down();
        break;
    case KEY_LEFT:
        move_cursor_left();
        break;
    case KEY_RIGHT:
        move_cursor_right();
        break;
    case 'h':
        move_cursor_left();
        break;
    case 'j':
        move_cursor_down();
        break;
    case 'k':
        move_cursor_up();
        break;
    case 'l':
        move_cursor_right();
        break;
    case KEY_PPAGE:
        page_up();
        break;
    case KEY_NPAGE:
        page_down();
        break;
    case 21: // Ctrl+U
        half_page_up();
        break;
    case 4: // Ctrl+D
        half_page_down();
        break;
    case 'i':
        enter_insert_mode();
        break;
    case 'I':
        clamp_cursor();
        tab->cursor.column = 0;
        enter_insert_mode();
        break;
    case 'a':
        enter_insert_mode();
        if (!tab->buffer.lines().empty())
        {
            clamp_cursor();
            int max_column = static_cast<int>(tab->buffer.lines()[tab->cursor.line].size());
            if (tab->cursor.column < max_column)
                tab->cursor.column++;
        }
        break;
    case 'A':
        enter_insert_mode();
        if (!tab->buffer.lines().empty())
        {
            clamp_cursor();
            tab->cursor.column = static_cast<int>(tab->buffer.lines()[tab->cursor.line].size());
        }
        break;
    case 'o':
    {
        clamp_cursor();
        const int line = tab->cursor.line;
        const int col = static_cast<int>(tab->buffer.lines()[line].size());
        tab->buffer.insert_newline(line, col);
        tab->cursor.line = line + 1;
        tab->cursor.column = 0;
        enter_insert_mode();
        break;
    }
    case 'O':
        clamp_cursor();
        tab->buffer.insert_empty_line(tab->cursor.line);
        tab->cursor.column = 0;
        enter_insert_mode();
        break;
    case 's':
        clamp_cursor();
        tab->buffer.delete_char_at(tab->cursor.line, tab->cursor.column);
        enter_insert_mode();
        break;
    case 'S':
        clamp_cursor();
        if (!tab->buffer.lines().empty())
        {
            while (!tab->buffer.lines()[tab->cursor.line].empty())
                tab->buffer.delete_char_at(tab->cursor.line, 0);
            tab->cursor.column = 0;
        }
        enter_insert_mode();
        break;
    case 'C':
        clamp_cursor();
        if (!tab->buffer.lines().empty())
        {
            while (tab->cursor.column < static_cast<int>(tab->buffer.lines()[tab->cursor.line].size()))
                tab->buffer.delete_char_at(tab->cursor.line, tab->cursor.column);
        }
        enter_insert_mode();
        break;
    case 'x':
        clamp_cursor();
        tab->buffer.delete_char_at(tab->cursor.line, tab->cursor.column);
        break;
    default:
        break;
    }

    clamp_cursor();
    update_scroll();
}

void Editor::handle_insert_input(int key)
{
    if (!tab)
        return;

    // jj → NORMAL (fast, no Esc needed)
    if (pending_j)
    {
        pending_j = false;
        if (key == 'j')
        {
            tab->buffer.delete_char_before(tab->cursor.line, tab->cursor.column);
            if (tab->cursor.column > 0)
                tab->cursor.column--;
            leave_insert_mode();
            clamp_cursor();
            update_scroll();
            return;
        }
    }

    switch (key)
    {
    case 27: // Esc / Ctrl+[
    case 3:  // Ctrl+C
        leave_insert_mode();
        break;
    case KEY_UP:
        pending_j = false;
        move_cursor_up();
        break;
    case KEY_DOWN:
        pending_j = false;
        move_cursor_down();
        break;
    case KEY_LEFT:
        pending_j = false;
        move_cursor_left();
        break;
    case KEY_RIGHT:
        pending_j = false;
        move_cursor_right();
        break;
    case KEY_PPAGE:
        pending_j = false;
        page_up();
        break;
    case KEY_NPAGE:
        pending_j = false;
        page_down();
        break;
    case KEY_BACKSPACE:
    case 127:
    case 8:
        pending_j = false;
        tab->buffer.delete_char_before(tab->cursor.line, tab->cursor.column);
        if (tab->cursor.column > 0)
            tab->cursor.column--;
        else if (tab->cursor.line > 0)
        {
            tab->cursor.line--;
            tab->cursor.column = static_cast<int>(tab->buffer.lines()[tab->cursor.line].size());
        }
        break;
    case KEY_DC:
        pending_j = false;
        tab->buffer.delete_char_at(tab->cursor.line, tab->cursor.column);
        break;
    case '\t':
        pending_j = false;
        tab->buffer.insert_char(tab->cursor.line, tab->cursor.column, '\t');
        tab->cursor.column++;
        break;
    case '\n':
    case KEY_ENTER:
        pending_j = false;
        tab->buffer.insert_newline(tab->cursor.line, tab->cursor.column);
        tab->cursor.line++;
        tab->cursor.column = 0;
        break;
    default:
        if (key >= 32 && key <= 126)
        {
            tab->buffer.insert_char(tab->cursor.line, tab->cursor.column, static_cast<char>(key));
            tab->cursor.column++;
            pending_j = (key == 'j');
        }
        else
        {
            pending_j = false;
        }
        break;
    }

    clamp_cursor();
    update_scroll();
}

void Editor::handle_input(int key)
{
    if (!tab)
        return;

    if (tab->mode == EditorMode::Normal)
        handle_normal_input(key);
    else
        handle_insert_input(key);
}

void Editor::set_cursor_position(int line, int column)
{
    if (!tab)
        return;

    tab->cursor = {.line = line, .column = column};
    tab->scroll_y = 0;
    tab->scroll_x = 0;
}

Cursor Editor::get_cursor() const
{
    return tab ? tab->cursor : Cursor{.line = 0, .column = 0};
}

int Editor::get_scroll_y() const
{
    return tab ? tab->scroll_y : 0;
}

Buffer &Editor::get_buffer()
{
    static Buffer empty;
    return tab ? tab->buffer : empty;
}

const Buffer &Editor::get_buffer() const
{
    static Buffer empty;
    return tab ? tab->buffer : empty;
}
