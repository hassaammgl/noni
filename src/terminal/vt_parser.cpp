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

void VtParser::handle_csi(unsigned char c)
{
    if (csi_buf_.empty() && (c == '?' || c == '>' || c == '=' || c == '!'))
    {
        csi_private_ = true;
        csi_buf_.push_back(static_cast<char>(c));
        return;
    }
    if ((c >= '0' && c <= '9') || c == ';' || c == ':')
    {
        csi_buf_.push_back(static_cast<char>(c));
        return;
    }
    if (c >= 0x20 && c <= 0x3F && c != '?')
    {
        // Intermediate — collect but we mostly ignore.
        csi_buf_.push_back(static_cast<char>(c));
        return;
    }

    // Final byte
    // Parse params from csi_buf_ (skip leading private marker).
    params_.clear();
    std::string nums;
    for (char ch : csi_buf_)
    {
        if (ch == '?' || ch == '>' || ch == '=' || ch == '!')
            continue;
        if (ch == ';' || ch == ':')
        {
            if (nums.empty())
                params_.push_back(0);
            else
            {
                params_.push_back(std::atoi(nums.c_str()));
                nums.clear();
            }
        }
        else if (std::isdigit(static_cast<unsigned char>(ch)))
            nums.push_back(ch);
    }
    if (!nums.empty())
        params_.push_back(std::atoi(nums.c_str()));
    else if (!csi_buf_.empty() && csi_buf_.back() == ';')
        params_.push_back(0);

    // Store final in csi_buf_ last char for dispatch — pass as member via final_c
    csi_buf_.push_back(static_cast<char>(c));
    dispatch_csi();
    state_ = State::Ground;
    csi_buf_.clear();
}

int VtParser::param(std::size_t i, int fallback) const
{
    if (i >= params_.size())
        return fallback;
    return params_[i] == 0 ? fallback : params_[i];
}

void VtParser::dispatch_csi()
{
    if (csi_buf_.empty())
        return;
    const char final = csi_buf_.back();

    if (csi_private_)
    {
        // Ignore DEC private modes (?h/?l) and similar for P15.
        return;
    }

    switch (final)
    {
    case 'A': // CUU
        screen_.move_cursor(-param(0, 1), 0);
        break;
    case 'B': // CUD
        screen_.move_cursor(param(0, 1), 0);
        break;
    case 'C': // CUF
        screen_.move_cursor(0, param(0, 1));
        break;
    case 'D': // CUB
        screen_.move_cursor(0, -param(0, 1));
        break;
    case 'E': // CNL
        screen_.move_cursor(param(0, 1), 0);
        screen_.carriage_return();
        break;
    case 'F': // CPL
        screen_.move_cursor(-param(0, 1), 0);
        screen_.carriage_return();
        break;
    case 'G': // CHA
        screen_.set_cursor(screen_.cursor_row(), param(0, 1) - 1);
        break;
    case 'H': // CUP
    case 'f': // HVP
        screen_.set_cursor(param(0, 1) - 1, param(1, 1) - 1);
        break;
    case 'J': // ED
        screen_.erase_in_display(params_.empty() ? 0 : params_[0]);
        break;
    case 'K': // EL
        screen_.erase_in_line(params_.empty() ? 0 : params_[0]);
        break;
    case 'L': // IL — deferred (needs proper insert-line)
    case 'M': // DL — deferred
        break;
    case 'S': // SU
        for (int i = 0; i < param(0, 1); ++i)
            screen_.index();
        break;
    case 'T': // SD
        for (int i = 0; i < param(0, 1); ++i)
            screen_.reverse_index();
        break;
    case 'd': // VPA
        screen_.set_cursor(param(0, 1) - 1, screen_.cursor_col());
        break;
    case 'm': // SGR
        apply_sgr();
        break;
    case 'n': // DSR — ignore (no reply yet)
        break;
    case 'r': // DECSTBM
        if (params_.empty())
            screen_.reset_scroll_region();
        else
            screen_.set_scroll_region(param(0, 1) - 1, param(1, screen_.rows()) - 1);
        break;
    case 's':
        screen_.save_cursor();
        break;
    case 'u':
        screen_.restore_cursor();
        break;
    default:
        break;
    }
}

void VtParser::apply_sgr()
{
    if (params_.empty())
        params_.push_back(0);

    auto &pen = screen_.pen();
    for (std::size_t i = 0; i < params_.size(); ++i)
    {
        const int p = params_[i];
        switch (p)
        {
        case 0:
            pen = {};
            break;
        case 1:
            pen.bold = true;
            break;
        case 2:
            pen.dim = true;
            break;
        case 4:
            pen.underline = true;
            break;
        case 7:
            pen.inverse = true;
            break;
        case 22:
            pen.bold = false;
            pen.dim = false;
            break;
        case 24:
            pen.underline = false;
            break;
        case 27:
            pen.inverse = false;
            break;
        case 39:
            pen.fg = 255;
            break;
        case 49:
            pen.bg = 255;
            break;
        default:
            if (p >= 30 && p <= 37)
                pen.fg = static_cast<std::uint8_t>(p - 30);
            else if (p >= 40 && p <= 47)
                pen.bg = static_cast<std::uint8_t>(p - 40);
            else if (p >= 90 && p <= 97)
                pen.fg = static_cast<std::uint8_t>(8 + (p - 90));
            else if (p >= 100 && p <= 107)
                pen.bg = static_cast<std::uint8_t>(8 + (p - 100));
            else if (p == 38 || p == 48)
            {
                // 256-color / RGB — consume and approximate.
                const bool is_fg = (p == 38);
                if (i + 1 < params_.size() && params_[i + 1] == 5 && i + 2 < params_.size())
                {
                    int idx = params_[i + 2];
                    if (idx < 0)
                        idx = 0;
                    if (idx > 15)
                        idx = idx % 16; // crude fold into 16
                    if (is_fg)
                        pen.fg = static_cast<std::uint8_t>(idx);
                    else
                        pen.bg = static_cast<std::uint8_t>(idx);
                    i += 2;
                }
                else if (i + 1 < params_.size() && params_[i + 1] == 2 && i + 4 < params_.size())
                {
                    // RGB → ignore precise, keep default-ish bold cue
                    i += 4;
                }
            }
            break;
        }
    }
}

void VtParser::handle_osc(unsigned char c)
{
    if (c == '\a' || (c == '\\' && !osc_buf_.empty() && osc_buf_.back() == 0x1b))
    {
        // BEL or ST (ESC\) terminator. If ESC\, strip trailing ESC.
        if (c == '\\' && !osc_buf_.empty() && osc_buf_.back() == 0x1b)
            osc_buf_.pop_back();

        // OSC 0/2 ; title
        if (osc_buf_.size() >= 2 && (osc_buf_[0] == '0' || osc_buf_[0] == '2') && osc_buf_[1] == ';')
            screen_.set_title(osc_buf_.substr(2));

        osc_buf_.clear();
        state_ = State::Ground;
        return;
    }
    if (c == 0x1b)
    {
        osc_buf_.push_back(static_cast<char>(c));
        return;
    }
    if (osc_buf_.size() < 4096)
        osc_buf_.push_back(static_cast<char>(c));
}
