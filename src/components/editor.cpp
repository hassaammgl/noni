#include <components/editor.hpp>
#include <syntax/syntax.hpp>
#include <ui/theme.hpp>
#include <utils/clipboard.hpp>
#include <utils/messages.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace
{
    int cp_display_width(char32_t cp, int display_col)
    {
        if (cp == U'\t')
            return TextMetrics::tab_width_at(display_col);
        return TextMetrics::codepoint_width(cp);
    }

    int cursor_display_col(const Window &w)
    {
        if (!w.has_buffer())
            return 0;
        const auto &lines = w.buffer().lines();
        if (lines.empty())
            return 0;
        const Cursor &c = w.cursor();
        if (c.line < 0 || c.line >= static_cast<int>(lines.size()))
            return 0;
        return TextMetrics::byte_to_display(
            lines[static_cast<std::size_t>(c.line)], static_cast<std::size_t>(c.column));
    }

    void sync_preferred_display(Window &w)
    {
        w.preferred_column() = cursor_display_col(w);
    }

    void sync_horizontal_scroll(Window &w, int pane_w)
    {
        if (pane_w <= 0)
            return;
        const int dcol = cursor_display_col(w);
        if (dcol < w.scroll_x())
            w.scroll_x() = dcol;
        if (dcol >= w.scroll_x() + pane_w)
            w.scroll_x() = dcol - pane_w + 1;
        if (w.scroll_x() < 0)
            w.scroll_x() = 0;
    }

    // Paint UTF-8 codepoints whose byte range overlaps [byte_lo, byte_hi) into
    // display viewport [sx, sx + pw). Uses display columns for screen x.
    void paint_byte_range(
        WINDOW *win,
        int row,
        int ox,
        std::string_view line,
        int byte_lo,
        int byte_hi,
        int sx,
        int pw,
        short color_pair)
    {
        if (!win || pw <= 0 || byte_lo >= byte_hi)
            return;

        int dcol = 0;
        std::size_t i = 0;
        wattron(win, COLOR_PAIR(color_pair));
        while (i < line.size() && dcol < sx + pw)
        {
            const auto [cp, n] = TextMetrics::decode(line, i);
            if (n == 0)
                break;
            const int w = cp_display_width(cp, dcol);
            const int i_end = static_cast<int>(i + n);

            if (i_end > byte_lo && static_cast<int>(i) < byte_hi && dcol + w > sx)
            {
                if (cp == U'\t')
                {
                    for (int t = 0; t < w; ++t)
                    {
                        const int screen_x = dcol + t - sx;
                        if (screen_x >= 0 && screen_x < pw)
                            mvwaddch(win, row, ox + screen_x, ' ');
                    }
                }
                else
                {
                    int screen_x = dcol - sx;
                    if (screen_x < 0)
                    {
                        // Wide glyph straddling left edge: fill occupied cells.
                        for (int t = 0; t < w; ++t)
                        {
                            const int sx_cell = dcol + t - sx;
                            if (sx_cell >= 0 && sx_cell < pw)
                                mvwaddch(win, row, ox + sx_cell, ' ');
                        }
                    }
                    else if (screen_x < pw)
                    {
                        mvwaddnstr(
                            win,
                            row,
                            ox + screen_x,
                            line.data() + i,
                            static_cast<int>(n));
                    }
                }
            }

            dcol += w;
            i += n;
        }
        wattroff(win, COLOR_PAIR(color_pair));
    }
}

void Editor::bind_core(EditorCore *core)
{
    core_ = core;
}

void Editor::bind(EditorTab *new_tab)
{
    tab = new_tab;
    pending_j = false;
    clear_pending();
}

void Editor::set_tab_switch_handlers(std::function<void()> next, std::function<void()> prev)
{
    on_next_tab_ = std::move(next);
    on_prev_tab_ = std::move(prev);
}

void Editor::set_search_handlers(std::function<void()> forward, std::function<void()> backward)
{
    on_search_forward_ = std::move(forward);
    on_search_backward_ = std::move(backward);
}

Window &Editor::win()
{
    return tab->active_window();
}

const Window &Editor::win() const
{
    return tab->active_window();
}

RegisterFile &Editor::registers()
{
    return core_->registers();
}

MarkTable &Editor::marks()
{
    return core_->marks();
}

JumpList &Editor::jumps()
{
    return core_->jumps();
}

void Editor::clear_pending()
{
    pending_g = false;
    pending_m = false;
    pending_jump_mark = false;
    pending_register = false;
    pending_op = PendingOperator::None;
    if (core_)
        registers().clear_pending();
}

bool Editor::in_visual() const
{
    return tab &&
           (tab->mode == EditorMode::Visual || tab->mode == EditorMode::VisualLine);
}

void Editor::leave_visual()
{
    if (!tab)
        return;
    tab->active_window().clear_selection();
    tab->mode = EditorMode::Normal;
}

void Editor::enter_visual_char()
{
    if (!tab)
        return;
    clear_pending();
    clamp_cursor();
    tab->mode = EditorMode::Visual;
    tab->active_window().enter_visual(SelectionKind::Character);
}

void Editor::enter_visual_line()
{
    if (!tab)
        return;
    clear_pending();
    clamp_cursor();
    tab->mode = EditorMode::VisualLine;
    tab->active_window().enter_visual(SelectionKind::Line);
}

std::uintptr_t Editor::current_buffer_id() const
{
    if (!tab || !tab->active_window().has_buffer())
        return 0;
    return reinterpret_cast<std::uintptr_t>(&tab->buffer());
}

void Editor::on_buffer_closed(std::uintptr_t buffer_id)
{
    marks().invalidate_buffer(buffer_id);
    jumps().invalidate_buffer(buffer_id);
}

void Editor::record_jump_from_here()
{
    if (!tab)
        return;
    jumps().push(current_buffer_id(), tab->cursor());
}

void Editor::jump_to(std::uintptr_t buffer_id, Cursor pos)
{
    if (!tab || buffer_id == 0)
        return;
    if (buffer_id != current_buffer_id())
        return; // same-tab only for now (no cross-buffer jump without BufferManager hook)
    tab->cursor() = pos;
    clamp_cursor();
    update_scroll();
}

void Editor::move_cursor_up()
{
    if (!tab)
        return;
    apply_motion(Motion::up(win()));
}

void Editor::move_cursor_down()
{
    if (!tab)
        return;
    apply_motion(Motion::down(win()));
}

void Editor::move_cursor_left()
{
    if (!tab)
        return;
    apply_motion(Motion::left(win()));
}

void Editor::move_cursor_right()
{
    if (!tab)
        return;
    apply_motion(Motion::right(win()));
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
    Cursor from = tab->cursor();
    tab->cursor().line = std::max(0, tab->cursor().line - step);
    tab->scroll_y() = std::max(0, tab->scroll_y() - step);
    {
        const auto &lines = tab->buffer().lines();
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
            tab->cursor().column = static_cast<int>(
                TextMetrics::display_to_byte(row, tab->active_window().preferred_column()));
        }
    }
    clamp_cursor();
    if (std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}

void Editor::page_down()
{
    if (!tab)
        return;
    clamp_cursor();
    const auto &lines = tab->buffer().lines();
    if (lines.empty())
        return;
    const int step = page_step();
    const int last = static_cast<int>(lines.size()) - 1;
    Cursor from = tab->cursor();
    tab->cursor().line = std::min(last, tab->cursor().line + step);
    tab->scroll_y() = std::min(
        std::max(0, last - std::max(0, height - 1)),
        tab->scroll_y() + step);
    {
        const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
        tab->cursor().column = static_cast<int>(
            TextMetrics::display_to_byte(row, tab->active_window().preferred_column()));
    }
    clamp_cursor();
    if (std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}

void Editor::half_page_up()
{
    if (!tab)
        return;
    clamp_cursor();
    const int step = std::max(1, page_step() / 2);
    Cursor from = tab->cursor();
    tab->cursor().line = std::max(0, tab->cursor().line - step);
    tab->scroll_y() = std::max(0, tab->scroll_y() - step);
    {
        const auto &lines = tab->buffer().lines();
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
            tab->cursor().column = static_cast<int>(
                TextMetrics::display_to_byte(row, tab->active_window().preferred_column()));
        }
    }
    clamp_cursor();
    if (std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}

void Editor::half_page_down()
{
    if (!tab)
        return;
    clamp_cursor();
    const auto &lines = tab->buffer().lines();
    if (lines.empty())
        return;
    const int step = std::max(1, page_step() / 2);
    const int last = static_cast<int>(lines.size()) - 1;
    Cursor from = tab->cursor();
    tab->cursor().line = std::min(last, tab->cursor().line + step);
    tab->scroll_y() = std::min(
        std::max(0, last - std::max(0, height - 1)),
        tab->scroll_y() + step);
    {
        const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
        tab->cursor().column = static_cast<int>(
            TextMetrics::display_to_byte(row, tab->active_window().preferred_column()));
    }
    clamp_cursor();
    if (std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}

void Editor::update_scroll()
{
    if (!tab || height <= 0)
        return;

    if (tab->cursor().line < tab->scroll_y())
        tab->scroll_y() = tab->cursor().line;

    if (tab->cursor().line >= tab->scroll_y() + height)
        tab->scroll_y() = tab->cursor().line - height + 1;

    if (tab->scroll_y() < 0)
        tab->scroll_y() = 0;

    sync_horizontal_scroll(tab->active_window(), width);
}

void Editor::clamp_cursor()
{
    if (!tab)
        return;

    tab->cursor() = clamp_cursor_to_lines(tab->cursor(), tab->buffer().lines());
}

void Editor::apply_motion(const MotionResult &motion, bool record_jump)
{
    if (!tab)
        return;
    Cursor from = tab->cursor();
    tab->cursor() = motion.dest;
    clamp_cursor();
    if (record_jump && std::abs(from.line - tab->cursor().line) > 1)
        jumps().push(current_buffer_id(), from);
}

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

void Editor::draw_selection_overlay(Window &w, int ox, int oy, int pw, int ph)
{
    if (!w.has_selection() || !window || pw <= 0 || ph <= 0)
        return;

    const auto &content = w.buffer().lines();
    const int sx = w.scroll_x();
    const int sy = w.scroll_y();

    for (int row = 0; row < ph; ++row)
    {
        const int line_index = sy + row;
        if (line_index < 0 || line_index >= static_cast<int>(content.size()))
            continue;

        const std::string &line = content[static_cast<std::size_t>(line_index)];

        if (w.selection().kind == SelectionKind::Line)
        {
            const int lo = std::min(w.selection().anchor.line, w.cursor().line);
            const int hi = std::max(w.selection().anchor.line, w.cursor().line);
            if (line_index < lo || line_index > hi)
                continue;

            paint_byte_range(
                window,
                oy + row,
                ox,
                line,
                0,
                static_cast<int>(line.size()),
                sx,
                pw,
                Theme::Selection);
            // Fill remainder of pane past EOL.
            const int line_dw = TextMetrics::line_display_width(line);
            if (line_dw < sx + pw)
            {
                const int fill_start = std::max(0, line_dw - sx);
                wattron(window, COLOR_PAIR(Theme::Selection));
                mvwhline(window, oy + row, ox + fill_start, ' ', pw - fill_start);
                wattroff(window, COLOR_PAIR(Theme::Selection));
            }
            continue;
        }

        // Character selection: paint selected codepoints by byte range.
        int dcol = 0;
        std::size_t i = 0;
        while (i < line.size() && dcol < sx + pw)
        {
            const auto [cp, n] = TextMetrics::decode(line, i);
            if (n == 0)
                break;
            if (w.is_selected(line_index, static_cast<int>(i)))
            {
                paint_byte_range(
                    window,
                    oy + row,
                    ox,
                    line,
                    static_cast<int>(i),
                    static_cast<int>(i + n),
                    sx,
                    pw,
                    Theme::Selection);
            }
            dcol += cp_display_width(cp, dcol);
            i += n;
        }
        // Visual EOL cell when selection includes end.
        const int eol_disp = TextMetrics::line_display_width(line);
        if (w.is_selected(line_index, static_cast<int>(line.size())) &&
            eol_disp >= sx && eol_disp < sx + pw)
        {
            wattron(window, COLOR_PAIR(Theme::Selection));
            mvwaddch(window, oy + row, ox + eol_disp - sx, ' ');
            wattroff(window, COLOR_PAIR(Theme::Selection));
        }
    }
}

void Editor::draw_search_highlight(Window &w, int ox, int oy, int pw, int ph)
{
    if (!core_ || !window || pw <= 0 || ph <= 0)
        return;

    const auto &st = core_->search();
    if (!st.active() || st.matches().empty())
        return;
    if (!w.has_buffer())
        return;
    if (st.buffer_id() != reinterpret_cast<std::uintptr_t>(&w.buffer()))
        return;

    const auto &content = w.buffer().lines();
    const int sx = w.scroll_x();
    const int sy = w.scroll_y();
    const int cur_idx = st.current_index();

    for (int mi = 0; mi < static_cast<int>(st.matches().size()); ++mi)
    {
        const auto &m = st.matches()[static_cast<std::size_t>(mi)];
        if (m.start.line != m.end.line)
            continue;
        if (m.start.line < sy || m.start.line >= sy + ph)
            continue;

        const int row = m.start.line - sy;
        if (m.start.line >= static_cast<int>(content.size()))
            continue;
        const std::string &line = content[static_cast<std::size_t>(m.start.line)];
        const short pair = (mi == cur_idx) ? Theme::SearchMatchCurrent : Theme::SearchMatch;
        paint_byte_range(
            window,
            oy + row,
            ox,
            line,
            m.start.column,
            m.end.column,
            sx,
            pw,
            pair);
    }
}

void Editor::draw_window_pane(Window &w, int ox, int oy, int pw, int ph, bool is_active)
{
    if (!window || !w.has_buffer() || pw <= 0 || ph <= 0)
        return;

    // Clamp scroll for this pane size.
    if (w.cursor().line < w.scroll_y())
        w.scroll_y() = w.cursor().line;
    if (w.cursor().line >= w.scroll_y() + ph)
        w.scroll_y() = w.cursor().line - ph + 1;
    if (w.scroll_y() < 0)
        w.scroll_y() = 0;
    sync_horizontal_scroll(w, pw);

    const auto &content = w.buffer().lines();
    w.buffer().sync_syntax();
    auto &syntax = w.buffer().syntax();

    const int sx = w.scroll_x();
    const int sy = w.scroll_y();

    for (int row = 0; row < ph; ++row)
    {
        const int line_index = sy + row;
        if (line_index >= static_cast<int>(content.size()))
            continue;

        const std::string &line = content[static_cast<std::size_t>(line_index)];
        const auto &tokens = syntax.tokens_for_line(line_index);
        const int line_len = static_cast<int>(line.size());

        paint_byte_range(
            window, oy + row, ox, line, 0, line_len, sx, pw, Theme::Editor);

        for (const auto &tok : tokens)
        {
            if (tok.kind == TokenKind::Text)
                continue;
            const int tok_start = tok.start;
            const int tok_end = std::min(tok.start + tok.length, line_len);
            if (tok_start >= tok_end)
                continue;
            paint_byte_range(
                window,
                oy + row,
                ox,
                line,
                tok_start,
                tok_end,
                sx,
                pw,
                Syntax::theme_pair(tok.kind));
        }
    }

    // Precedence: base syntax → search → selection → cursor.
    draw_search_highlight(w, ox, oy, pw, ph);
    draw_selection_overlay(w, ox, oy, pw, ph);

    if (is_active)
    {
        const int cy = w.cursor().line - w.scroll_y();
        const int cx = cursor_display_col(w) - w.scroll_x();
        if (cy >= 0 && cy < ph && cx >= 0 && cx < pw)
            wmove(window, oy + cy, ox + cx);
    }
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

    if (core_)
        core_->set_editor_area(width, height);

    auto rects = tab->layout.compute_rects(0, 0, width, height);
    Window *active = tab->layout.active();

    // Draw split separators.
    for (const auto &[w, r] : rects)
    {
        (void)w;
        // Vertical separator to the right of pane if another pane starts at x+width+1
        for (const auto &[w2, r2] : rects)
        {
            if (r2.x == r.x + r.width + 1 && r2.y < r.y + r.height && r.y < r2.y + r2.height)
            {
                wattron(window, COLOR_PAIR(Theme::Border));
                for (int y = std::max(r.y, r2.y); y < std::min(r.y + r.height, r2.y + r2.height); ++y)
                    mvwaddch(window, y, r.x + r.width, ACS_VLINE);
                wattroff(window, COLOR_PAIR(Theme::Border));
            }
            if (r2.y == r.y + r.height + 1 && r2.x < r.x + r.width && r.x < r2.x + r2.width)
            {
                wattron(window, COLOR_PAIR(Theme::Border));
                mvwhline(window, r.y + r.height, r.x, ACS_HLINE, r.width);
                wattroff(window, COLOR_PAIR(Theme::Border));
            }
        }
    }

    for (auto &[w, r] : rects)
    {
        if (!w)
            continue;
        draw_window_pane(*w, r.x, r.y, r.width, r.height, w == active);
    }
}

EditorMode Editor::get_mode() const
{
    return tab ? tab->mode : EditorMode::Normal;
}

std::string Editor::get_mode_label() const
{
    switch (get_mode())
    {
    case EditorMode::Insert:
        return "INSERT";
    case EditorMode::Visual:
        return "VISUAL";
    case EditorMode::VisualLine:
        return "V-LINE";
    default:
        return "NORMAL";
    }
}

void Editor::enter_insert_mode()
{
    if (!tab)
        return;
    pending_j = false;
    clear_pending();
    if (in_visual())
        leave_visual();
    clamp_cursor();
    if (!tab->buffer().is_edit_active())
        tab->buffer().begin_edit(tab->cursor().line, tab->cursor().column);
    tab->mode = EditorMode::Insert;
}

void Editor::leave_insert_mode()
{
    if (!tab)
        return;

    pending_j = false;
    if (tab->mode != EditorMode::Insert)
        return;

    if (tab->cursor().column > 0)
    {
        const auto &lines = tab->buffer().lines();
        if (!lines.empty())
        {
            const auto &row = lines[static_cast<std::size_t>(tab->cursor().line)];
            tab->cursor().column = static_cast<int>(
                TextMetrics::prev_cp(row, static_cast<std::size_t>(tab->cursor().column)));
        }
    }
    tab->mode = EditorMode::Normal;
    tab->buffer().end_edit(tab->cursor().line, tab->cursor().column);
    sync_preferred_display(tab->active_window());
}

void Editor::enter_normal_mode()
{
    if (!tab)
        return;

    clear_pending();
    if (tab->mode == EditorMode::Insert)
        leave_insert_mode();
    else if (in_visual())
        leave_visual();
    else
        tab->mode = EditorMode::Normal;
}

void Editor::begin_buffer_edit()
{
    if (!tab)
        return;
    clamp_cursor();
    tab->buffer().begin_edit(tab->cursor().line, tab->cursor().column);
}

void Editor::end_buffer_edit()
{
    if (!tab)
        return;
    clamp_cursor();
    tab->buffer().end_edit(tab->cursor().line, tab->cursor().column);
}

void Editor::split_insert_edit()
{
    if (!tab || tab->mode != EditorMode::Insert)
        return;
    end_buffer_edit();
    begin_buffer_edit();
}

bool Editor::paste_clipboard()
{
    if (!tab)
        return false;

    const auto text = Clipboard::read();
    if (!text || text->empty())
        return false;

    RegisterValue value;
    value.text = *text;
    value.type = RegisterType::Character;
    registers().set_unnamed(std::move(value));
    return paste_register(true);
}

bool Editor::undo()
{
    if (!tab)
        return false;

    if (tab->mode == EditorMode::Insert)
        leave_insert_mode();
    else if (in_visual())
        leave_visual();
    else if (tab->buffer().is_edit_active())
        end_buffer_edit();

    clear_pending();

    const UndoResult result = tab->buffer().undo();
    if (!result.ok)
        return false;

    tab->cursor().line = result.cursor_line;
    tab->cursor().column = result.cursor_col;
    clamp_cursor();
    sync_preferred_display(tab->active_window());
    update_scroll();
    return true;
}

bool Editor::redo()
{
    if (!tab)
        return false;

    if (tab->mode == EditorMode::Insert)
        leave_insert_mode();
    else if (in_visual())
        leave_visual();
    else if (tab->buffer().is_edit_active())
        end_buffer_edit();

    clear_pending();

    const UndoResult result = tab->buffer().redo();
    if (!result.ok)
        return false;

    tab->cursor().line = result.cursor_line;
    tab->cursor().column = result.cursor_col;
    clamp_cursor();
    sync_preferred_display(tab->active_window());
    update_scroll();
    return true;
}

static bool is_motion_key(int key)
{
    switch (key)
    {
    case 'h':
    case 'j':
    case 'k':
    case 'l':
    case 'w':
    case 'b':
    case 'e':
    case '0':
    case '^':
    case '$':
    case 'G':
    case KEY_UP:
    case KEY_DOWN:
    case KEY_LEFT:
    case KEY_RIGHT:
        return true;
    default:
        return false;
    }
}

static MotionResult motion_from_key(Window &win, int key)
{
    switch (key)
    {
    case KEY_UP:
    case 'k':
        return Motion::up(win);
    case KEY_DOWN:
    case 'j':
        return Motion::down(win);
    case KEY_LEFT:
    case 'h':
        return Motion::left(win);
    case KEY_RIGHT:
    case 'l':
        return Motion::right(win);
    case 'w':
        return Motion::word_forward(win);
    case 'b':
        return Motion::word_backward(win);
    case 'e':
        return Motion::word_end(win);
    case '0':
        return Motion::line_start(win);
    case '^':
        return Motion::first_non_blank(win);
    case '$':
        return Motion::line_end(win);
    case 'G':
        return Motion::file_end(win);
    default:
        return {};
    }
}

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

void Editor::handle_normal_input(int key)
{
    if (!tab)
        return;

    // Register prefix: "a
    if (key == '"')
    {
        pending_register = true;
        pending_m = false;
        pending_jump_mark = false;
        return;
    }

    if (pending_register)
    {
        pending_register = false;
        if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || key == '"')
            registers().set_pending(static_cast<char>(key));
        return;
    }

    if (pending_m)
    {
        pending_m = false;
        if (key >= 'a' && key <= 'z')
            marks().set(static_cast<char>(key), current_buffer_id(), tab->cursor());
        return;
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
        return;
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
            return;
        }
        if (key == 't' && on_next_tab_)
        {
            on_next_tab_();
            return;
        }
        if (key == 'T' && on_prev_tab_)
        {
            on_prev_tab_();
            return;
        }
        clear_pending();
        return;
    }

    // Operator-pending: second key
    if (pending_op != PendingOperator::None)
    {
        if (key == 27)
        {
            clear_pending();
            return;
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
            return;
        }

        if (key == 'g')
        {
            pending_g = true;
            return;
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
            return;
        }

        clear_pending();
        return;
    }

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

    clamp_cursor();
    update_scroll();
}

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

void Editor::handle_input(int key)
{
    if (!tab)
        return;

    if (tab->mode == EditorMode::Normal)
        handle_normal_input(key);
    else if (in_visual())
        handle_visual_input(key);
    else
        handle_insert_input(key);
}

void Editor::set_cursor_position(int line, int column)
{
    if (!tab)
        return;

    record_jump_from_here();
    tab->cursor() = {.line = line, .column = column};
    clamp_cursor();
    sync_preferred_display(tab->active_window());
    tab->scroll_y() = 0;
    tab->scroll_x() = 0;
}

Cursor Editor::get_cursor() const
{
    return tab ? tab->cursor() : Cursor{.line = 0, .column = 0};
}

int Editor::get_scroll_y() const
{
    return tab ? tab->scroll_y() : 0;
}

Buffer &Editor::get_buffer()
{
    static Buffer empty;
    return tab ? tab->buffer() : empty;
}

const Buffer &Editor::get_buffer() const
{
    static Buffer empty;
    return tab ? tab->buffer() : empty;
}
