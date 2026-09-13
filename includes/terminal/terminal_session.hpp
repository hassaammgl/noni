#pragma once

#include <terminal/pty_session.hpp>
#include <terminal/terminal_screen.hpp>
#include <terminal/vt_parser.hpp>

#include <optional>
#include <string>

enum class TerminalSessionState
{
    Idle,
    Starting,
    Running,
    Exited,
    Failed,
    Closing,
};

struct TerminalSessionConfig
{
    std::string shell; // empty → $SHELL → /bin/sh
    std::string cwd;
    int scrollback = 5000;
};

// Owns PTY + emulator state. UI-independent.
class TerminalSession
{
public:
    TerminalSession();

    void set_config(TerminalSessionConfig cfg);
    const TerminalSessionConfig &config() const { return config_; }

    bool start(int rows, int cols);
    void stop();
    bool alive() const;

    TerminalSessionState state() const { return state_; }
    std::optional<int> exit_status() const { return exit_status_; }

    void resize(int rows, int cols);
    void write_bytes(const char *data, std::size_t n);
    void write_byte(char c);

    // Drain PTY → parser → screen. Returns true if screen may have changed.
    bool pump();

    TerminalScreen &screen() { return screen_; }
    const TerminalScreen &screen() const { return screen_; }

    void clear_screen();

    // View scrollback: 0 = follow live bottom; >0 = lines scrolled up.
    void set_view_scroll(int lines);
    int view_scroll() const { return view_scroll_; }
    void scroll_view(int delta);
    void follow_live();

private:
    TerminalSessionConfig config_;
    PtySession pty_;
    TerminalScreen screen_;
    VtParser parser_;
    TerminalSessionState state_ = TerminalSessionState::Idle;
    std::optional<int> exit_status_;
    int view_scroll_ = 0;
    int rows_ = 0;
    int cols_ = 0;
};
