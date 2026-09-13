#pragma once

#include <terminal/terminal_session.hpp>
#include <ui/UIComponent.hpp>

#include <string>

// ncurses view over a TerminalSession. Does not parse VT or own the shell.
class TerminalPanel : public UIComponent
{
private:
    TerminalSession session_;
    bool focused_ = false;
    bool visible_ = false;

    int view_rows() const;
    short color_for_cell(const TerminalCell &cell) const;
    void draw_cell(int row, int col, const TerminalCell &cell);

public:
    void draw() override;

    void set_visible(bool v);
    bool is_visible() const;
    void set_focused(bool v);
    bool is_focused() const;

    void apply_config(const TerminalSessionConfig &cfg);
    void set_cwd(const std::string &path);

    void ensure_started();
    void stop();
    bool poll();

    void on_resized();
    void handle_input(int key);

    void clear_screen();
    void scroll_up();
    void scroll_down();

    TerminalSession &session() { return session_; }
    const TerminalSession &session() const { return session_; }
};
