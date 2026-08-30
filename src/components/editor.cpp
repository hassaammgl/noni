#include <components/editor.hpp>
#include <ui/theme.hpp>
#include <utils/logger.hpp>
#include <format>

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
        const auto &lines = buffer.lines();
        if (!lines.empty())
            cursor.column = static_cast<int>(lines[cursor.line].size());
    }
}
void Editor::move_cursor_right()
{
    const auto &lines = buffer.lines();
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
    const auto &lines = buffer.lines();
    if (lines.empty())
    {
        cursor.line = 0;
        cursor.column = 0;
        return;
    }

    if (cursor.line < 0)
        cursor.line = 0;
    if (cursor.line >= static_cast<int>(lines.size()))
        cursor.line = static_cast<int>(lines.size()) - 1;

    int max_column = static_cast<int>(lines[cursor.line].size());
    if (cursor.column < 0)
        cursor.column = 0;
    if (cursor.column > max_column)
        cursor.column = max_column;
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
    const auto &content = buffer.lines();
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
    Logger::debug(std::format("Editor input key={}", key));

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
    case KEY_BACKSPACE:
    case 127:
    case 8:
    {
        buffer.delete_char_before(cursor.line, cursor.column);
        if (cursor.column > 0)
            cursor.column--;
        else if (cursor.line > 0)
        {
            cursor.line--;
            cursor.column = static_cast<int>(buffer.lines()[cursor.line].size());
        }
        break;
    }
    case '\n':
    case KEY_ENTER:
        buffer.insert_newline(cursor.line, cursor.column);
        cursor.line++;
        cursor.column = 0;
        break;
    default:
        if (key >= 32 && key <= 126)
        {
            buffer.insert_char(cursor.line, cursor.column, static_cast<char>(key));
            cursor.column++;
        }
        break;
    }

    clamp_cursor();
    update_scroll();
}

void Editor::set_cursor_position(int line, int column)
{
    cursor = {.line = line, .column = column};
    Logger::debug(std::format("Editor cursor set to line={} column={}", line, column));
}

Cursor Editor::get_cursor() const
{
    return cursor;
}
