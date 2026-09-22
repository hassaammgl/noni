#include "editor_impl.hpp"

void Editor::handle_visual_input(int key)
{
    if (!tab)
        return;

    if (key == 27)
    {
        leave_visual();
        clear_pending();
        return;
    }

    if (key == 'v')
    {
        if (tab->mode == EditorMode::Visual)
            leave_visual();
        else
            enter_visual_char();
        return;
    }
    if (key == 'V')
    {
        if (tab->mode == EditorMode::VisualLine)
            leave_visual();
        else
            enter_visual_line();
        return;
    }

    if (key == 'd' || key == 'x')
    {
        operator_on_visual(PendingOperator::Delete, false);
        return;
    }
    if (key == 'c' || key == 's')
    {
        operator_on_visual(PendingOperator::Change, true);
        return;
    }
    if (key == 'y')
    {
        operator_on_visual(PendingOperator::Yank, false);
        return;
    }

    if (key == 'p' || key == 'P')
    {
        leave_visual();
        paste_register(key == 'p');
        return;
    }

    if (is_motion_key(key))
    {
        apply_motion(motion_from_key(win(), key), key == 'G');
        clamp_cursor();
        update_scroll();
        return;
    }

    if (key == 'g')
    {
        pending_g = true;
        return;
    }

    if (pending_g)
    {
        pending_g = false;
        if (key == 'g')
        {
            apply_motion(Motion::file_start(win()), true);
            clamp_cursor();
            update_scroll();
        }
        return;
    }

    if (key == 4) // Ctrl-D
    {
        half_page_down();
        return;
    }
    if (key == 21) // Ctrl-U
    {
        half_page_up();
        return;
    }
    if (key == KEY_PPAGE)
    {
        page_up();
        return;
    }
    if (key == KEY_NPAGE)
    {
        page_down();
        return;
    }
}
