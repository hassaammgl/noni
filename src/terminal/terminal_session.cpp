#include <terminal/terminal_session.hpp>
#include <utils/logger.hpp>

#include <algorithm>
#include <format>

TerminalSession::TerminalSession() : parser_(screen_) {}

void TerminalSession::set_config(TerminalSessionConfig cfg)
{
    config_ = std::move(cfg);
    screen_.set_scrollback_limit(config_.scrollback);
}

bool TerminalSession::start(int rows, int cols)
{
    state_ = TerminalSessionState::Starting;
    exit_status_.reset();
    view_scroll_ = 0;
    rows_ = std::max(1, rows);
    cols_ = std::max(1, cols);

    screen_.resize(rows_, cols_);
    screen_.reset();
    screen_.set_scrollback_limit(config_.scrollback);
    parser_.reset();

    if (!pty_.start(rows_, cols_, config_.cwd, config_.shell))
    {
        state_ = TerminalSessionState::Failed;
        Logger::error("TerminalSession: PTY start failed");
        return false;
    }

    state_ = TerminalSessionState::Running;
    return true;
}

void TerminalSession::stop()
{
    if (state_ == TerminalSessionState::Idle)
        return;
    state_ = TerminalSessionState::Closing;
    pty_.stop();
    exit_status_ = pty_.exit_status();
    state_ = exit_status_ ? TerminalSessionState::Exited : TerminalSessionState::Idle;
    if (pty_.lifecycle() == PtyLifecycle::Failed)
        state_ = TerminalSessionState::Failed;
}

bool TerminalSession::alive() const
{
    return state_ == TerminalSessionState::Running && pty_.alive();
}

void TerminalSession::resize(int rows, int cols)
{
    rows = std::max(1, rows);
    cols = std::max(1, cols);
    rows_ = rows;
    cols_ = cols;
    screen_.resize(rows, cols);
    if (alive())
        pty_.resize(rows, cols);
}

void TerminalSession::write_bytes(const char *data, std::size_t n)
{
    if (!alive())
        return;
    follow_live();
    pty_.write_bytes(data, n);
}

void TerminalSession::write_byte(char c)
{
    write_bytes(&c, 1);
}

bool TerminalSession::pump()
{
    const std::string chunk = pty_.take_output();
    bool changed = false;
    if (!chunk.empty())
    {
        parser_.feed(chunk);
        changed = true;
    }

    if (state_ == TerminalSessionState::Running && !pty_.alive())
    {
        exit_status_ = pty_.exit_status();
        state_ = TerminalSessionState::Exited;
        changed = true;
    }
    return changed;
}

void TerminalSession::clear_screen()
{
    screen_.erase_in_display(2);
    screen_.set_cursor(0, 0);
    follow_live();
}

void TerminalSession::set_view_scroll(int lines)
{
    const int max_scroll = std::max(0, screen_.scrollback_lines());
    view_scroll_ = std::clamp(lines, 0, max_scroll);
}

void TerminalSession::scroll_view(int delta)
{
    set_view_scroll(view_scroll_ + delta);
}

void TerminalSession::follow_live()
{
    view_scroll_ = 0;
}
