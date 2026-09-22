#include "editor_impl.hpp"

void Editor::handle_normal_input(int key)
{
    if (!tab)
        return;
    if (handle_normal_prefix(key))
        return;
    handle_normal_commands(key);
    clamp_cursor();
    update_scroll();
}

void Editor::handle_normal_commands(int key)
{
    switch (key)
    {
    case KEY_UP:
    case 'k':
        apply_motion(Motion::up(win()));
        break;
    case KEY_DOWN:
    case 'j':
        apply_motion(Motion::down(win()));
        break;
    case KEY_LEFT:
    case 'h':
        apply_motion(Motion::left(win()));
        break;
    case KEY_RIGHT:
    case 'l':
        apply_motion(Motion::right(win()));
        break;
    case 'w':
        apply_motion(Motion::word_forward(win()));
        break;
    case 'b':
        apply_motion(Motion::word_backward(win()));
        break;
    case 'e':
        apply_motion(Motion::word_end(win()));
        break;
    case '0':
        apply_motion(Motion::line_start(win()));
        break;
    case '^':
        apply_motion(Motion::first_non_blank(win()));
        break;
    case '$':
        apply_motion(Motion::line_end(win()));
        break;
    case 'G':
        apply_motion(Motion::file_end(win()), true);
        break;
    case 'g':
        pending_g = true;
        break;
    case KEY_PPAGE:
        page_up();
        break;
    case KEY_NPAGE:
        page_down();
        break;
    case 4: // Ctrl-D
        half_page_down();
        break;
    case 21: // Ctrl-U
        half_page_up();
        break;
    case 'v':
        enter_visual_char();
        break;
    case 'V':
        enter_visual_line();
        break;
    case '/':
        if (on_search_forward_)
            on_search_forward_();
        break;
    case '?':
        if (on_search_backward_)
            on_search_backward_();
        break;
    case 'n':
        if (core_)
        {
            const std::string err = core_->search_next(false);
            if (!err.empty())
                Messages::info(err);
            clamp_cursor();
            update_scroll();
        }
        break;
    case 'N':
        if (core_)
        {
            const std::string err = core_->search_next(true);
            if (!err.empty())
                Messages::info(err);
            clamp_cursor();
            update_scroll();
        }
        break;
    case 'd':
        pending_op = PendingOperator::Delete;
        break;
    case 'c':
        pending_op = PendingOperator::Change;
        break;
    case 'y':
        pending_op = PendingOperator::Yank;
        break;
    case 'm':
        pending_m = true;
        break;
    case '\'':
        pending_jump_mark = true;
        break;
    default:
        handle_normal_edits(key);
        break;
    }
}
