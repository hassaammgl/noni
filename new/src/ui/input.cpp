// P2.1 — see input.hpp.
#include "ui/input.hpp"

#include <format>

namespace noni::ui {

namespace {

// KEY_* -> KeyKind table (values are terminfo-stable in ncurses).
Key decode_key_code(wint_t code) {
    switch (code) {
    case KEY_UP: return Key{KeyKind::ArrowUp, 0};
    case KEY_DOWN: return Key{KeyKind::ArrowDown, 0};
    case KEY_LEFT: return Key{KeyKind::ArrowLeft, 0};
    case KEY_RIGHT: return Key{KeyKind::ArrowRight, 0};
    case KEY_HOME: return Key{KeyKind::Home, 0};
    case KEY_END: return Key{KeyKind::End, 0};
    case KEY_PPAGE: return Key{KeyKind::PageUp, 0};
    case KEY_NPAGE: return Key{KeyKind::PageDown, 0};
    case KEY_DC: return Key{KeyKind::Delete, 0};
    case KEY_RESIZE: return Key{KeyKind::Resize, 0};
    case KEY_BACKSPACE: return Key{KeyKind::Backspace, 0};
    case k_paste_begin_code: return Key{KeyKind::PasteBegin, 0};
    case k_paste_end_code: return Key{KeyKind::PasteEnd, 0};
    default: return Key{KeyKind::Unknown, static_cast<char32_t>(code)};
    }
}

// Control-byte and ASCII specials (wide-char path, is_key_code == false).
constexpr char32_t k_ctrl_offset = 0x60; // 0x01..0x1A folds to 'a'..'z'

Key decode_char(wint_t code) {
    const char32_t c = static_cast<char32_t>(code);
    switch (c) {
    case 0x08: return Key{KeyKind::CtrlChar, U'h'}; // Ctrl+H
    case 0x09: return Key{KeyKind::Tab, 0};
    case 0x0A: return Key{KeyKind::Enter, 0}; // Enter AND Ctrl+J (same byte)
    case 0x0D: return Key{KeyKind::Enter, 0};
    case 0x1B: return Key{KeyKind::Esc, 0};
    default:
        if (c >= 0x01 && c <= 0x1A) {
            return Key{KeyKind::CtrlChar, c + k_ctrl_offset};
        }
        if (c == 0x7F) {
            return Key{KeyKind::Backspace, 0};
        }
        return Key{KeyKind::Char, c};
    }
}

} // namespace

Key KeyDecoder::feed(bool is_key_code, wint_t code) {
    // Paste content: RAW bytes until the PasteEnd code (nothing is special
    // inside a paste — not \n, not 0x7F, not ESC).
    if (st_ == St::PasteData) {
        if (!is_key_code) {
            return Key{KeyKind::Char, static_cast<char32_t>(code)};
        }
        const Key k = decode_key_code(code);
        if (k.kind == KeyKind::PasteEnd) {
            st_ = St::Idle;
        }
        return k;
    }

    // CSI-in-progress assembly (bracketed paste + unknown markers).
    if (st_ == St::CsiDigits) {
        if (code >= L'0' && code <= L'9') {
            csi_num_ = csi_num_ * 10 + (code - L'0');
            return Key{KeyKind::None, 0};
        }
        st_ = St::Idle;
        if (code == L'~') {
            if (csi_num_ == 200) {
                st_ = St::PasteData; // fallback path enters paste mode too
                return Key{KeyKind::PasteBegin, 0};
            }
            if (csi_num_ == 201) {
                return Key{KeyKind::PasteEnd, 0};
            }
            // Unknown CSI: degrade to the number as data (vim-style).
            const std::uint32_t n = csi_num_;
            csi_num_ = 0;
            return Key{KeyKind::Unknown, static_cast<char32_t>(n)};
        }
        // Malformed sequence: flush and treat this byte normally.
        csi_num_ = 0;
    }

    if (is_key_code) {
        const Key k = decode_key_code(code);
        if (k.kind == KeyKind::PasteBegin) {
            st_ = St::PasteData;
        } else if (k.kind == KeyKind::PasteEnd) {
            st_ = St::Idle;
        }
        return k;
    }
    const char32_t c = static_cast<char32_t>(code);
    if (st_ == St::EscSeen) {
        st_ = St::Idle;
        if (c == L'[') {
            st_ = St::CsiDigits; // ESC [ N ~ markers (+ unknown CSIs)
            csi_num_ = 0;
            return Key{KeyKind::None, 0};
        }
        // Other bytes after Esc = Alt-prefix: both keys already surfaced.
    }
    if (c == 0x1B) {
        st_ = St::EscSeen; // Esc emitted NOW; '[' may arm a CSI on the next
        return Key{KeyKind::Esc, 0};
    }
    return decode_char(code);
}

std::vector<Key> decode_stream(const std::vector<Wch>& codes) {
    KeyDecoder d;
    std::vector<Key> out;
    out.reserve(codes.size());
    for (const auto& [is_key, code] : codes) {
        const Key k = d.feed(is_key, code);
        if (k.kind != KeyKind::None) {
            out.push_back(k);
        }
    }
    return out;
}

std::string key_debug(const Key& k) {
    const char* name = "?";
    switch (k.kind) {
    case KeyKind::None: name = "None"; break;
    case KeyKind::Char: name = "Char"; break;
    case KeyKind::Esc: name = "Esc"; break;
    case KeyKind::Enter: name = "Enter"; break;
    case KeyKind::Backspace: name = "Backspace"; break;
    case KeyKind::Tab: name = "Tab"; break;
    case KeyKind::CtrlChar:
        return std::format("Ctrl+{}", static_cast<char>(k.cp));
    case KeyKind::ArrowUp: name = "Up"; break;
    case KeyKind::ArrowDown: name = "Down"; break;
    case KeyKind::ArrowLeft: name = "Left"; break;
    case KeyKind::ArrowRight: name = "Right"; break;
    case KeyKind::Home: name = "Home"; break;
    case KeyKind::End: name = "End"; break;
    case KeyKind::PageUp: name = "PageUp"; break;
    case KeyKind::PageDown: name = "PageDown"; break;
    case KeyKind::Delete: name = "Delete"; break;
    case KeyKind::Resize: name = "Resize"; break;
    case KeyKind::PasteBegin: name = "PasteBegin"; break;
    case KeyKind::PasteEnd: name = "PasteEnd"; break;
    case KeyKind::Unknown: name = "Unknown"; break;
    }
    return k.cp != 0 ? std::format("{}:{:x}", name, static_cast<unsigned>(k.cp))
                     : std::format("{}", name);
}

eresult<Key> KeyReader::next() {
    wint_t code = 0;
    for (;;) {
        // wget_wch on OUR window (keypad + define_key registered there);
        // plain get_wch would read stdscr and bypass all resolution.
        const int rc = wget_wch(win_, &code);
        if (rc == KEY_CODE_YES) {
            return dec_.feed(true, code);
        }
        if (rc != OK) {
            return std::unexpected(Error{"input: get_wch failed"});
        }
        const Key k = dec_.feed(false, code);
        if (k.kind != KeyKind::None) {
            return k;
        }
        // CSI assembly consumed the byte: keep reading.
    }
}

} // namespace noni::ui
