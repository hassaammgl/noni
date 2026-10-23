#pragma once

#include <terminal/terminal_screen.hpp>

#include <cstdint>
#include <string>
#include <vector>

// Byte-stream VT/ANSI parser → TerminalScreen. Holds partial-sequence state.
class VtParser
{
public:
    explicit VtParser(TerminalScreen &screen);

    void reset();
    void feed(const char *data, std::size_t n);
    void feed(const std::string &s) { feed(s.data(), s.size()); }

private:
    enum class State : std::uint8_t
    {
        Ground,
        Escape,
        Csi,
        Osc,
        Utf8,
    };

    void feed_byte(unsigned char c);
    void handle_ground(unsigned char c);
    void handle_escape(unsigned char c);
    void handle_csi(unsigned char c);
    void handle_osc(unsigned char c);
    void dispatch_csi();
    void dispatch_esc(unsigned char c);
    void apply_sgr();
    int param(std::size_t i, int fallback = 0) const;

    TerminalScreen &screen_;
    State state_ = State::Ground;

    std::string csi_buf_;
    std::vector<int> params_;
    bool csi_private_ = false;

    std::string osc_buf_;

    // UTF-8 assembly
    char32_t utf8_cp_ = 0;
    int utf8_need_ = 0;
};
