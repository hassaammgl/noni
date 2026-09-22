#include "editor_impl.hpp"

bool Editor::handle_normal_prefix(int key)
{
    // Register prefix: "a
    if (key == '"')
    {
        pending_register = true;
        pending_m = false;
        pending_jump_mark = false;
        return true;
    }

    if (pending_register)
    {
        pending_register = false;
        if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || key == '"')
            registers().set_pending(static_cast<char>(key));
        return true;
    }

    if (pending_m)
    {
        pending_m = false;
        if (key >= 'a' && key <= 'z')
            marks().set(static_cast<char>(key), current_buffer_id(), tab->cursor());
        return true;
    }

    if (pending_jump_mark)
    {
        pending_jump_mark = false;
        if (key >= 'a' && key <= 'z')
        {
            const Mark *mark = marks().get(static_cast<char>(key));
            if (mark)
            {
                record_jump_from_here();
                jump_to(mark->buffer_id, mark->pos);
            }
        }
        return true;
    }

    if (pending_g)
    {
        pending_g = false;
        if (key == 'g')
        {
            if (pending_op != PendingOperator::None)
            {
                const MotionResult motion = Motion::file_start(win());
                execute_operator(pending_op, visual_or_motion_range(motion), pending_op == PendingOperator::Change);
                clear_pending();
            }
            else
            {
                apply_motion(Motion::file_start(win()), true);
            }
            clamp_cursor();
            update_scroll();
            return true;
        }
        if (key == 't' && on_next_tab_)
        {
            on_next_tab_();
            return true;
        }
        if (key == 'T' && on_prev_tab_)
        {
            on_prev_tab_();
            return true;
        }
        clear_pending();
        return true;
    }

    // Operator-pending: second key
    if (pending_op != PendingOperator::None)
    {
        if (key == 27)
        {
            clear_pending();
            return true;
        }

        // Linewise operators: dd / cc / yy
        if ((pending_op == PendingOperator::Delete && key == 'd') ||
            (pending_op == PendingOperator::Change && key == 'c') ||
            (pending_op == PendingOperator::Yank && key == 'y'))
        {
            TextRange range = normalize_line_range(
                tab->cursor(), tab->cursor(), static_cast<int>(tab->buffer().lines().size()));
            const PendingOperator op = pending_op;
            clear_pending();
            execute_operator(op, range, op == PendingOperator::Change);
            clamp_cursor();
            update_scroll();
            return true;
        }

        if (key == 'g')
        {
            pending_g = true;
            return true;
        }

        if (is_motion_key(key))
        {
            const MotionResult motion = motion_from_key(win(), key);
            const PendingOperator op = pending_op;
            clear_pending();
            execute_operator(op, visual_or_motion_range(motion), op == PendingOperator::Change);
            if (op == PendingOperator::Yank)
                apply_motion(motion); // vim restores cursor on yank — keep simple: stay
            clamp_cursor();
            update_scroll();
            return true;
        }

        clear_pending();
        return true;
    }
    return false;
}
