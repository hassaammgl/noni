#include <terminal/vt_parser.hpp>

#include <cctype>
#include <cstdlib>

VtParser::VtParser(TerminalScreen &screen) : screen_(screen) {}

void VtParser::reset()
{
    state_ = State::Ground;
    csi_buf_.clear();
    params_.clear();
    csi_private_ = false;
    osc_buf_.clear();
    utf8_cp_ = 0;
    utf8_need_ = 0;
}

void VtParser::feed(const char *data, std::size_t n)
{
    if (!data || n == 0)
        return;
    for (std::size_t i = 0; i < n; ++i)
        feed_byte(static_cast<unsigned char>(data[i]));
}

void VtParser::feed_byte(unsigned char c)
{
    switch (state_)
    {
    case State::Utf8:
        if ((c & 0xC0) != 0x80)
        {
            // Invalid continuation — resync.
            state_ = State::Ground;
            feed_byte(c);
            return;
        }
        utf8_cp_ = (utf8_cp_ << 6) | (c & 0x3F);
        --utf8_need_;
        if (utf8_need_ == 0)
        {
            state_ = State::Ground;
            screen_.put_codepoint(utf8_cp_);
        }
        return;
    case State::Escape:
        handle_escape(c);
        return;
    case State::Csi:
        handle_csi(c);
        return;
    case State::Osc:
        handle_osc(c);
        return;
    case State::Ground:
    default:
        handle_ground(c);
        return;
    }
}

void VtParser::handle_ground(unsigned char c)
{
    if (c == 0x1b)
    {
        state_ = State::Escape;
        return;
    }
    if (c == '\n' || c == '\r' || c == '\b' || c == '\t' || c == '\a')
    {
        screen_.put_codepoint(c);
        return;
    }
    if (c == 0x0e || c == 0x0f) // SO/SI — ignore charset shifts
        return;
    if (c < 0x20)
        return;

    if (c < 0x80)
    {
        screen_.put_codepoint(c);
        return;
    }

    // UTF-8 lead
    if ((c & 0xE0) == 0xC0)
    {
        utf8_cp_ = c & 0x1F;
        utf8_need_ = 1;
        state_ = State::Utf8;
        return;
    }
    if ((c & 0xF0) == 0xE0)
    {
        utf8_cp_ = c & 0x0F;
        utf8_need_ = 2;
        state_ = State::Utf8;
        return;
    }
    if ((c & 0xF8) == 0xF0)
    {
        utf8_cp_ = c & 0x07;
        utf8_need_ = 3;
        state_ = State::Utf8;
        return;
    }
}

void VtParser::handle_escape(unsigned char c)
{
    if (c == '[')
    {
        state_ = State::Csi;
        csi_buf_.clear();
        params_.clear();
        csi_private_ = false;
        return;
    }
    if (c == ']')
    {
        state_ = State::Osc;
        osc_buf_.clear();
        return;
    }
    // ESC D / M / E / 7 / 8 etc.
    dispatch_esc(c);
    state_ = State::Ground;
}

void VtParser::dispatch_esc(unsigned char c)
{
    switch (c)
    {
    case 'D': // IND
        screen_.index();
        break;
    case 'M': // RI
        screen_.reverse_index();
        break;
    case 'E': // NEL
        screen_.newline();
        break;
    case '7': // DECSC
        screen_.save_cursor();
        break;
    case '8': // DECRC
        screen_.restore_cursor();
        break;
    case 'c': // RIS
        screen_.reset();
        break;
    default:
        break; // ignore unknown
    }
}

