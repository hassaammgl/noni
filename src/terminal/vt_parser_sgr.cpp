#include <terminal/vt_parser.hpp>

#include <cctype>
#include <cstdlib>

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
