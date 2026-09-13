#pragma once

#include <editor/editor_core.hpp>
#include <editor/editor_tab.hpp>
#include <editor/motion.hpp>
#include <lsp/completion.hpp>
#include <ui/UIComponent.hpp>

#include <functional>
#include <string>

enum class PendingOperator
{
    None,
    Delete,
    Change,
    Yank,
};

class Editor : public UIComponent
{
private:
    EditorCore *core_ = nullptr;
    EditorTab *tab = nullptr;
    bool pending_j = false;
    bool pending_g = false;
    bool pending_m = false;
    bool pending_jump_mark = false;
    bool pending_register = false;
    PendingOperator pending_op = PendingOperator::None;
    std::function<void()> on_next_tab_;
    std::function<void()> on_prev_tab_;
    std::function<void()> on_search_forward_;
    std::function<void()> on_search_backward_;

    Window &win();
    const Window &win() const;

    void move_cursor_up();
    void move_cursor_down();
    void move_cursor_left();
    void move_cursor_right();
    void page_up();
    void page_down();
    void half_page_up();
    void half_page_down();
    void update_scroll();
    void clamp_cursor();
    void handle_normal_input(int key);
    void handle_visual_input(int key);
    void handle_insert_input(int key);
    void leave_insert_mode();
    int page_step() const;
    void begin_buffer_edit();
    void end_buffer_edit();
    void split_insert_edit();

    void clear_pending();
    bool in_visual() const;
    void leave_visual();
    void enter_visual_char();
    void enter_visual_line();

    void apply_motion(const MotionResult &motion, bool record_jump = false);
    bool execute_operator(PendingOperator op, const TextRange &range, bool enter_insert);
    bool operator_on_visual(PendingOperator op, bool enter_insert);
    bool yank_range(const TextRange &range);
    bool delete_range(const TextRange &range);
    bool paste_register(bool after);
    TextRange visual_or_motion_range(const MotionResult &motion) const;

    std::uintptr_t current_buffer_id() const;
    void jump_to(std::uintptr_t buffer_id, Cursor pos);
    void record_jump_from_here();

    void draw_window_pane(Window &w, int ox, int oy, int pw, int ph, bool is_active);
    void draw_selection_overlay(Window &w, int ox, int oy, int pw, int ph);
    void draw_search_highlight(Window &w, int ox, int oy, int pw, int ph);
    void draw_diagnostics_overlay(Window &w, int ox, int oy, int pw, int ph);

public:
    void bind_core(EditorCore *core);
    void bind(EditorTab *tab);

    void set_tab_switch_handlers(std::function<void()> next, std::function<void()> prev);
    void set_search_handlers(std::function<void()> forward, std::function<void()> backward);

    Cursor get_cursor() const;
    EditorMode get_mode() const;
    std::string get_mode_label() const;

    void enter_insert_mode();
    void enter_normal_mode();

    bool paste_clipboard();
    bool apply_completion(const CompletionItem &item);
    bool undo();
    bool redo();

    Buffer &get_buffer();
    const Buffer &get_buffer() const;

    RegisterFile &registers();
    MarkTable &marks();
    JumpList &jumps();

    void on_buffer_closed(std::uintptr_t buffer_id);

    void draw() override;
    void handle_input(int key);

    void set_cursor_position(int line, int column);
    int get_scroll_y() const;
};
