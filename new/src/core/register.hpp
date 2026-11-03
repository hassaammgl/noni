#pragma once
// P4.2 — the register payload operators move text through: the seed of the
// full register model (P5.1 adds the numbered/readonly registers, uppercase
// append, and the small-delete history). Shape stays: lines + linewise flag,
// so 'p'/'P' (P5.2) can replay a yank verbatim.
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace noni {

struct Register {
    std::vector<std::string> lines;
    bool linewise = false;

    [[nodiscard]] bool empty() const { return lines.empty(); }
    // The moved text as one string, lines joined with '\n'.
    [[nodiscard]] std::string text() const;
};

class RegisterBank {
public:
    // '"x' names a register: 'a'..'z' (uppercase APPENDS, vim-style) or
    // '"'/0 for the unnamed one. Every named write also lands in the unnamed
    // register (vim: a named yank is also the unnamed yank).
    void set(char32_t name, Register value);
    [[nodiscard]] const Register& get(char32_t name) const;
    [[nodiscard]] const Register& unnamed() const { return unnamed_; }

private:
    Register unnamed_;
    std::array<Register, 26> named_{};
};

} // namespace noni
