#include <components/editor.hpp>
#include <ui/theme.hpp>

void Editor::move_cursor_up()
{
    if (cursor.line > 0)
    {
        cursor.line--;
    }
    clamp_cursor();
}
void Editor::move_cursor_down()
{
    cursor.line++;
    clamp_cursor();
}
void Editor::move_cursor_left()
{
    if (cursor.column > 0)
    {
        cursor.column--;
        return;
    }
    if (cursor.line > 0)
    {
        cursor.line--;
        auto lines = buffer.read_buffer();
        if (!lines.empty())
            cursor.column = static_cast<int>(lines[cursor.line].size());
    }
}
void Editor::move_cursor_right()
{
    auto lines = buffer.read_buffer();
    if (lines.empty())
        return;
    clamp_cursor();
    int max_column = static_cast<int>(lines[cursor.line].size());
    if (cursor.column < max_column)
    {
        cursor.column++;
        return;
    }

    if (cursor.line + 1 < static_cast<int>(lines.size()))
    {
        cursor.line++;
        cursor.column = 0;
    }
}

void Editor::update_scroll()
{
    if (height <= 0)
        return;

    if (cursor.line < scroll_y)
        scroll_y = cursor.line;

    if (cursor.line >= scroll_y + height)
        scroll_y = cursor.line - height + 1;

    if (scroll_y < 0)
        scroll_y = 0;
}

void Editor::clamp_cursor()
{
    auto lines = buffer.read_buffer();
    if (lines.empty())
    {
        cursor.line = 0;
        cursor.column = 0;
        return;
    }
    if (cursor.line < 0)
    {
        cursor.line = 0;
    }
    if (cursor.line >= static_cast<int>(lines.size()))
        cursor.line = static_cast<int>(lines.size()) - 1;

    int max_column = static_cast<int>(lines[cursor.line].size());
    if (cursor.column < 0)
    {
        cursor.column = 0;
    }
    if (cursor.column > max_column)
    {
        cursor.column = max_column;
    }
}

void Editor::draw()
{
    if (!window)
        return;

    werase(window);
    leaveok(window, FALSE);
    wbkgd(window, COLOR_PAIR(Theme::Editor));
    clamp_cursor();
    update_scroll();
    auto content = buffer.read_buffer();
    for (int row = 0; row < height; ++row)
    {
        int line_index = scroll_y + row;
        if (line_index >= static_cast<int>(content.size()))
            break;

        mvwprintw(window, row, 0,
             content[line_index].c_str());
    }
    wmove(window, cursor.line - scroll_y, cursor.column);
    wnoutrefresh(window);
}

void Editor::handle_input(int key)
{
    switch (key)
    {
    case KEY_UP:
        this->move_cursor_up();
        break;
    case KEY_DOWN:
        this->move_cursor_down();
        break;
    case KEY_LEFT:
        this->move_cursor_left();
        break;
    case KEY_RIGHT:
        this->move_cursor_right();
        break;

    default:
        break;
    }
    this->update_scroll();
}

void Editor::set_cursor_position(int line, int column)
{
    this->cursor = {.line = line, .column = column};
}

Cursor Editor::get_cursor() const
{
    return cursor;
}