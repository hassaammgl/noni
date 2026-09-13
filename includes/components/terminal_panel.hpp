#pragma once

#include <terminal/pty_session.hpp>
#include <ui/UIComponent.hpp>

#include <string>
#include <vector>

class TerminalPanel : public UIComponent
{
private:
    PtySession pty;
    std::vector<std::string> lines{""};
    int cursor_col = 0;
    int scroll_back = 0; // 0 = follow bottom
    bool focused = false;
    bool visible = false;
    std::string cwd;
    static constexpr int kMaxLines = 5000;

    void ingest(const std::string &chunk);
    void ensure_line();
    void newline();
    void put_char(char ch);
    void strip_and_put(const std::string &chunk);
    int view_rows() const;

public:
    void draw() override;

    void set_visible(bool v);
    bool is_visible() const;
    void set_focused(bool v);
    bool is_focused() const;

    void set_cwd(const std::string &path);
    void ensure_started();
    void stop();
    bool poll(); // drain pty output; true if content changed

    void on_resized();
    void handle_input(int key);
};
