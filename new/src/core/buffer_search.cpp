// P1.7 — see buffer_search.hpp.
#include "core/buffer_search.hpp"

#include <cassert>

#include <algorithm>

namespace noni {

namespace {

// AGENTS.md: RAII wrapper for every C-library resource — regex_t included.
class Regex {
public:
    Regex() = default;
    ~Regex() {
        if (compiled_) {
            regfree(&re_);
        }
    }
    Regex(const Regex&) = delete;
    Regex& operator=(const Regex&) = delete;
    // Move keeps ownership unique; source stays uncompiled (no double regfree).
    Regex(Regex&& o) noexcept : re_(o.re_), compiled_(o.compiled_) {
        o.compiled_ = false;
    }

    [[nodiscard]] static eresult<Regex> compile(std::string_view pattern,
                                                Query q) {
        Regex r;
        int flags = REG_EXTENDED;
        if (!q.case_sensitive) {
            flags |= REG_ICASE;
        }
        std::string final = q.regex ? std::string{pattern}
                                    : escape_literals(pattern);
        if (q.whole_word) {
            final = "\\b" + final + "\\b"; // glibc extension (documented)
        }
        const int code = regcomp(&r.re_, final.c_str(), flags);
        if (code != 0) {
            const std::size_t buf = regerror(code, &r.re_, nullptr, 0);
            std::string msg(buf > 0 ? buf - 1 : 16, '\0');
            if (buf > 0) {
                regerror(code, &r.re_, msg.data(), msg.size() + 1);
            }
            return std::unexpected(Error{"bad regex: " + msg});
        }
        r.compiled_ = true;
        return r;
    }

    // First match within [start, len) of `line` (bytes). REG_STARTEND keeps
    // NUL bytes inside the line observable. False = no match.
    [[nodiscard]] bool exec(const std::string& line, std::size_t start,
                            regmatch_t out[10], int eflags = 0) const {
        assert(compiled_);
        if (start > line.size()) {
            return false;
        }
        out[0].rm_so = static_cast<regoff_t>(start);
        out[0].rm_eo = static_cast<regoff_t>(line.size());
        return regexec(&re_, line.c_str(), 10, out, REG_STARTEND | eflags) == 0;
    }

private:
    // Escapes every ERE metacharacter for literal-pattern queries.
    [[nodiscard]] static std::string escape_literals(std::string_view p) {
        std::string out;
        out.reserve(p.size() * 2);
        for (const char c : p) {
            switch (c) {
            case '\\': case '.': case '^': case '$': case '[':
            case ']': case '|': case '(': case ')': case '*':
            case '+': case '?': case '{': case '}':
                out += '\\';
                [[fallthrough]];
            default:
                out += c;
            }
        }
        return out;
    }

    regex_t re_{};
    bool compiled_ = false;
};

// Expands the replacement template against one match. `&` = whole matched
// text, `\N` = capture group N (unset -> empty), `\\&` -> literal &,
// `\\\\` -> literal backslash.
std::string expand_repl(const regmatch_t (&m)[10], const std::string& line,
                        std::string_view repl) {
    std::string out;
    out.reserve(repl.size() + 16);
    for (std::size_t i = 0; i < repl.size(); ++i) {
        const char c = repl[i];
        if (c == '&') {
            out += line.substr(static_cast<std::size_t>(m[0].rm_so),
                               static_cast<std::size_t>(m[0].rm_eo - m[0].rm_so));
        } else if (c == '\\' && i + 1 < repl.size()) {
            const char n = repl[i + 1];
            if (n >= '1' && n <= '9') {
                const int g = n - '0';
                if (m[g].rm_so >= 0) {
                    out += line.substr(static_cast<std::size_t>(m[g].rm_so),
                                       static_cast<std::size_t>(m[g].rm_eo - m[g].rm_so));
                }
                ++i;
            } else if (n == '&') {
                out += '&';
                ++i;
            } else if (n == '\\') {
                out += '\\';
                ++i;
            } else {
                out += '\\';
            }
        } else {
            out += c;
        }
    }
    return out;
}

// All non-overlapping matches (first chain) in one line. REG_NOTBOL makes `^`
// anchor at the TRUE line start, not at each scan tail (POSIX eflag).
std::vector<regmatch_t> matches_in(const Regex& re, const std::string& line) {
    std::vector<regmatch_t> out;
    std::size_t pos = 0;
    for (;;) {
        regmatch_t m[10];
        if (!re.exec(line, pos, m, pos > 0 ? REG_NOTBOL : 0)) {
            break;
        }
        const std::size_t start = static_cast<std::size_t>(m[0].rm_so);
        const std::size_t len = static_cast<std::size_t>(m[0].rm_eo - m[0].rm_so);
        out.push_back(m[0]);
        if (len == 0) {
            pos = start + 1; // advance past an empty match
        } else {
            pos = start + len;
        }
        if (pos > line.size()) {
            break;
        }
    }
    return out;
}

} // namespace

eresult<std::vector<Match>>
find_all(const std::vector<std::string>& lines, std::string_view pattern,
         Query q) {
    if (pattern.empty()) {
        return std::unexpected(Error{"search: empty pattern"});
    }
    auto compiled = Regex::compile(pattern, q);
    if (!compiled) {
        return std::unexpected(compiled.error());
    }

    std::vector<Match> out;
    for (std::uint32_t li = 0; li < lines.size(); ++li) {
        for (const auto& m : matches_in(*compiled, lines[li])) {
            out.push_back(Match{li, static_cast<std::size_t>(m.rm_so),
                                static_cast<std::size_t>(m.rm_eo - m.rm_so)});
        }
        if (lines[li].empty()) {
            continue; // nothing to search; keeps the loop uniform
        }
    }
    return out;
}

eresult<std::size_t>
replace(std::vector<std::string>& lines, std::string_view pattern,
        std::string_view repl, Query q, bool all) {
    if (pattern.empty()) {
        return std::unexpected(Error{"search: empty pattern"});
    }
    auto compiled = Regex::compile(pattern, q);
    if (!compiled) {
        return std::unexpected(compiled.error());
    }

    std::size_t count = 0;
    for (std::uint32_t li = 0; li < lines.size(); ++li) {
        const std::string src = lines[li];
        std::string result;
        result.reserve(src.size());
        std::size_t pos = 0;
        bool wrote = false;
        while (pos <= src.size()) {
            regmatch_t m[10];
            if (!compiled->exec(src, pos, m, pos > 0 ? REG_NOTBOL : 0)) {
                break;
            }
            const std::size_t so = static_cast<std::size_t>(m[0].rm_so);
            const std::size_t eo = static_cast<std::size_t>(m[0].rm_eo);
            result += src.substr(pos, so - pos);
            result += expand_repl(m, src, repl);
            ++count;
            wrote = true;
            if (eo == so) {
                if (eo < src.size()) {
                    result += src[eo]; // empty match: keep the skipped char
                }
                pos = eo + 1;
            } else {
                pos = eo;
            }
            if (pos > src.size() || (!all && count == 1)) {
                break;
            }
        }
        if (wrote) {
            result += src.substr(std::min(pos, src.size()));
            lines[li] = std::move(result);
        }
        if (!all && count == 1) {
            break;
        }
    }
    return count;
}

} // namespace noni
