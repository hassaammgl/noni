// P4.2 — see register.hpp.
#include "core/register.hpp"

#include <algorithm>

namespace noni {
namespace {

// 0 for anything that is not a valid register name.
[[nodiscard]] int slot_for(const char32_t name) {
    if (name >= U'a' && name <= U'z') {
        return static_cast<int>(name - U'a');
    }
    if (name >= U'A' && name <= U'Z') {
        return static_cast<int>(name - U'A');
    }
    return -1;
}

[[nodiscard]] bool is_named(const char32_t name) {
    return (name >= U'a' && name <= U'z') || (name >= U'A' && name <= U'Z');
}

[[nodiscard]] bool is_append(const char32_t name) {
    return name >= U'A' && name <= U'Z';
}

} // namespace

std::string Register::text() const {
    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) {
            out += '\n';
        }
        out += lines[i];
    }
    return out;
}

void RegisterBank::set(const char32_t name, Register value) {
    // vim: reading the unnamed register is a snapshot; writes always refresh it.
    unnamed_ = value;
    const int slot = slot_for(name);
    if (!is_named(name) || slot < 0) {
        return;
    }
    if (is_append(name) && !named_[static_cast<std::size_t>(slot)].empty()) {
        // TODO(P5.1): uppercase append joins charwise text without a newline
        // and linewise text with one; P4 only needs the named slot to exist.
        Register& dst = named_[static_cast<std::size_t>(slot)];
        if (dst.linewise || value.linewise) {
            dst.lines.insert(dst.lines.end(), value.lines.begin(),
                             value.lines.end());
        } else if (!value.lines.empty()) {
            dst.lines.back() += value.lines.front();
            dst.lines.insert(dst.lines.end(), value.lines.begin() + 1,
                             value.lines.end());
        }
        dst.linewise = dst.linewise || value.linewise;
        unnamed_ = dst;
        return;
    }
    named_[static_cast<std::size_t>(slot)] = std::move(value);
}

const Register& RegisterBank::get(const char32_t name) const {
    const int slot = slot_for(name);
    if (slot < 0) {
        return unnamed_;
    }
    return named_[static_cast<std::size_t>(slot)];
}

} // namespace noni
