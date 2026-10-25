#pragma once
// P1.7 — pure buffer search/replace over a line vector (used by the / prompt
// machinery later and by :s/// in P7). No ncurses, no Buffer ownership.
//
// DEVIATION NOTE (documented per AGENTS.md): the ROADMAP named std::regex
// (ECMAScript). std::regex reports bad patterns by THROWING, which contradicts
// the project's no-exceptions rule, so we use POSIX regcomp/regexec (REG_
// EXTENDED) behind an RAII wrapper instead. Word boundaries use the glibc `\b`
// extension (Linux/glibc target platform).
#include <cstddef>
#include <cstdint>
#include <regex.h>
#include <string>
#include <string_view>
#include <vector>

#include "core/pos.hpp"

namespace noni {

// A hit: `byte_start`/`byte_len` are raw byte offsets within lines[line] (safe
// for NUL-containing lines; never cp-based).
struct Match {
    std::uint32_t line = 0;
    std::size_t byte_start = 0;
    std::size_t byte_len = 0;
};

struct Query {
    bool regex = false;        // false = literal text (no metacharacters)
    bool case_sensitive = true;
    bool whole_word = false;   // glibc \\b around the (already built) pattern
};

[[nodiscard]] eresult<std::vector<Match>> find_all(
    const std::vector<std::string>& lines, std::string_view pattern, Query q);

// Substitutes in place. Returns the number of substitutions performed.
// `all` = every match in every line; otherwise only the FIRST match overall.
// Replacement expansion: `&` = whole match, `\1`\..`\9` = capture groups (ERE
// `( )`), `\\&` = literal `&`, `\\\\` = literal backslash. Capture refs in
// literal mode are literals except the documented escapes.
[[nodiscard]] eresult<std::size_t> replace(std::vector<std::string>& lines,
                                           std::string_view pattern,
                                           std::string_view repl, Query q,
                                           bool all);

} // namespace noni
