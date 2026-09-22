#include <terminal/vt_parser.hpp>

#include <cctype>
#include <cstdlib>

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

