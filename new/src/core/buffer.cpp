// P1.2 — see buffer.hpp.
#include "core/buffer.hpp"

#include <cassert>
#include <cstdio>
#include <string_view>
#include <utility>

#include <algorithm>

#include "core/text.hpp"
#include "core/undo.hpp"

namespace noni {

namespace {
// Wraps a raw FILE* with the AGENTS.md RAII rule (no bare handles held).
struct FileGuard {
    std::FILE* p;
    ~FileGuard() {
        if (p != nullptr) {
            std::fclose(p);
        }
    }
};
} // namespace

Buffer::Buffer(std::vector<std::string> lines)
    : lines_(std::move(lines)) {
    if (lines_.empty()) {
        lines_.push_back("");
    }
}

std::vector<std::string> split_file_content(std::string_view data) {
    std::vector<std::string> lines;
    lines.reserve(16);
    std::size_t start = 0;
    while (start <= data.size()) {
        const std::size_t nl = data.find('\n', start);
        if (nl == std::string_view::npos) {
            std::string_view last = data.substr(start);
            if (!last.empty() && last.back() == '\r') {
                last.remove_suffix(1); // CRLF file whose last line lacks '\n'
            }
            lines.emplace_back(last);
            break;
        }
        std::string_view piece = data.substr(start, nl - start);
        if (!piece.empty() && piece.back() == '\r') {
            piece.remove_suffix(1); // CRLF -> LF
        }
        lines.emplace_back(piece);
        start = nl + 1;
        if (start == data.size()) {
            break; // trailing '\n': no implicit extra empty line
        }
    }
    return lines; // guaranteed "" if data is empty -> ctor keeps >= 1 line
}

std::string join_file_content(const std::vector<std::string>& lines) {
    assert(!lines.empty());
    std::string out;
    for (const auto& line : lines) {
        out += line;
        out += '\n';
    }
    return out;
}

eresult<Buffer> Buffer::load(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return std::unexpected(Error{"open: no such file: " + path.string()});
    }

    FileGuard guard{std::fopen(path.c_str(), "rb")};
    if (guard.p == nullptr) {
        return std::unexpected(Error{"open: cannot open: " + path.string()});
    }

    std::string data;
    char chunk[65536];
    for (;;) {
        const std::size_t n = std::fread(chunk, 1, sizeof chunk, guard.p);
        if (n == 0) {
            break;
        }
        data.append(chunk, n);
        if (std::ferror(guard.p) != 0) {
            return std::unexpected(Error{"read: i/o error on " + path.string()});
        }
    }

    Buffer buf{split_file_content({data.data(), data.size()})};
    buf.path_ = path;
    return buf;
}

eresult<void> Buffer::save(const std::filesystem::path& path) const {
    FileGuard guard{std::fopen(path.c_str(), "wb")};
    if (guard.p == nullptr) {
        return std::unexpected(
            Error{"write: cannot open for writing: " + path.string()});
    }
    for (const auto& line : lines_) {
        if (!line.empty()
            && std::fwrite(line.data(), 1, line.size(), guard.p) != line.size()) {
            return std::unexpected(Error{"write: short write on " + path.string()});
        }
        if (std::fwrite("\n", 1, 1, guard.p) != 1) {
            return std::unexpected(Error{"write: short write on " + path.string()});
        }
    }
    if (std::fflush(guard.p) != 0) {
        return std::unexpected(Error{"write: flush failed on " + path.string()});
    }
    return ok();
}

void Buffer::set_lines(std::vector<std::string> lines) {
    if (lines.empty()) {
        lines.push_back("");
    }
    lines_ = std::move(lines);
    dirty_ = true;
    ++revision_;
}
// --- edit primitives (P1.3) --------------------------------------------------

void Buffer::apply_change(const TextChange& change) {
    const auto first = lines_.begin()
                       + static_cast<std::ptrdiff_t>(change.first_line);
    const auto last = first + static_cast<std::ptrdiff_t>(change.old_lines.size());
    lines_.erase(first, last);
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(change.first_line),
                  change.new_lines.begin(), change.new_lines.end());
    dirty_ = true;
    ++revision_;
    undo_.push(change); // fresh edits from the present truncate the redo branch
}

void Buffer::apply_inverse(const TextChange& change) {
    const auto first = lines_.begin()
                       + static_cast<std::ptrdiff_t>(change.first_line);
    const auto last = first + static_cast<std::ptrdiff_t>(change.new_lines.size());
    // Internal precondition: undo replays against the exact state the change
    // left behind (shared invariant of the line-span model).
    assert(std::equal(change.new_lines.begin(), change.new_lines.end(), first, last)
           && "undo/redo baseline drift");
    lines_.erase(first, last);
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(change.first_line),
                  change.old_lines.begin(), change.old_lines.end());
    dirty_ = true;
    ++revision_;
}

eresult<Buffer::UndoResult> Buffer::undo() {
    auto unit = undo_.pop_undo();
    if (!unit) {
        return std::unexpected(Error{"undo: nothing to undo"});
    }
    for (auto it = unit->changes.rbegin(); it != unit->changes.rend(); ++it) {
        apply_inverse(*it); // inverses in REVERSE order
    }
    const UndoResult res{unit->changes.front().cur_before};
    undo_.push_redo(std::move(*unit));
    return res;
}

eresult<Buffer::UndoResult> Buffer::redo() {
    auto unit = undo_.pop_redo();
    if (!unit) {
        return std::unexpected(Error{"redo: nothing to redo"});
    }
    for (const auto& change : unit->changes) {
        // Re-apply forward. Reuse apply_change WITHOUT re-recording:
        // record_text_change would push a new unit and clear redo. So splice
        // manually (apply_inverse's assert inverted).
        const auto first = lines_.begin()
                           + static_cast<std::ptrdiff_t>(change.first_line);
        const auto last =
            first + static_cast<std::ptrdiff_t>(change.old_lines.size());
        assert(std::equal(change.old_lines.begin(), change.old_lines.end(),
                          first, last)
               && "redo baseline drift");
        lines_.erase(first, last);
        lines_.insert(lines_.begin()
                          + static_cast<std::ptrdiff_t>(change.first_line),
                      change.new_lines.begin(), change.new_lines.end());
        dirty_ = true;
        ++revision_;
    }
    const UndoResult res{unit->changes.back().cur_after};
    undo_.push_redo_return(std::move(*unit));
    return res;
}

void Buffer::record_text_change(const TextChange& change) {
    apply_change(change);
}

namespace {
// Clamp helpers over the current line table.
std::uint32_t clamp_line(const std::vector<std::string>& lines, std::uint32_t line) {
    if (lines.empty()) {
        return 0;
    }
    const std::uint32_t last = static_cast<std::uint32_t>(lines.size() - 1);
    return line > last ? last : line;
}
} // namespace

Pos Buffer::insert_char(Pos p, char32_t cp) {
    const std::uint32_t line = clamp_line(lines_, p.line);
    const std::string before = lines_[line];
    const std::size_t at = cp_to_byte(before, p.col);
    const std::string bytes = encode_utf8(cp);

    std::string after = before;
    after.insert(after.begin() + static_cast<std::ptrdiff_t>(at), bytes.begin(),
                 bytes.end());

    TextChange ch;
    ch.kind = EditKind::InsertChar;
    ch.first_line = line;
    ch.old_lines.push_back(before);
    ch.new_lines.push_back(after);
    const Pos result{line, p.col + 1};
    ch.cur_before = Pos{line, p.col};
    ch.cur_after = result;
    apply_change(ch);
    return result;
}

Pos Buffer::insert_text(Pos p, std::string_view text) {
    const std::uint32_t line = clamp_line(lines_, p.line);
    const std::string before = lines_[line];
    const std::size_t at = cp_to_byte(before, p.col);
    const std::uint32_t col = byte_to_cp(before, at);

    // Split `text` on '\n' (a trailing '\n' yields a final "" segment, which
    // correctly opens a new line for paste).
    std::vector<std::string> segs;
    segs.reserve(8);
    std::size_t seg_start = 0;
    while (seg_start <= text.size()) {
        const std::size_t nl = text.find('\n', seg_start);
        if (nl == std::string_view::npos) {
            segs.emplace_back(text.substr(seg_start));
            break;
        }
        segs.emplace_back(text.substr(seg_start, nl - seg_start));
        seg_start = nl + 1;
        if (seg_start == text.size()) {
            segs.emplace_back("");
            break;
        }
    }

    const std::string head{std::string_view{before}.substr(0, at)};
    const std::string tail{std::string_view{before}.substr(at)};

    std::vector<std::string> after;
    after.reserve(segs.size());
    after.push_back(head + segs[0]);
    for (std::size_t i = 1; i < segs.size(); ++i) {
        after.push_back(segs[i]);
    }
    if (segs.size() > 1) {
        after.back() += tail; // last segment keeps the old suffix
    }

    TextChange ch;
    ch.kind = EditKind::Bulk; // paste/multi-line never merges into typing runs
    ch.first_line = line;
    ch.old_lines.push_back(before);
    ch.new_lines = std::move(after);

    const Pos result =
        (segs.size() == 1)
            ? Pos{line, col + static_cast<std::uint32_t>(line_len_cp(segs[0]))}
        : segs.back().empty()
            ? Pos{line + static_cast<std::uint32_t>(segs.size() - 1), 0}
            : Pos{line + static_cast<std::uint32_t>(segs.size() - 1),
                  static_cast<std::uint32_t>(line_len_cp(segs.back()))};
    ch.cur_before = Pos{line, col};
    ch.cur_after = result;
    apply_change(ch);
    return result;
}

Pos Buffer::erase_char(Pos p) {
    const std::uint32_t line = clamp_line(lines_, p.line);
    const std::string before = lines_[line];
    const std::size_t at = cp_to_byte(before, p.col);
    if (at >= before.size()) {
        return Pos{line, p.col}; // end of line: no wrap
    }
    const auto dec = decode_utf8(before, at);
    const std::size_t len = dec ? dec->second : std::size_t{1};

    std::string after = before;
    after.erase(after.begin() + static_cast<std::ptrdiff_t>(at),
                after.begin() + static_cast<std::ptrdiff_t>(at + len));

    TextChange ch;
    ch.kind = EditKind::EraseChar;
    ch.first_line = line;
    ch.old_lines.push_back(before);
    ch.new_lines.push_back(after);
    ch.cur_before = Pos{line, p.col};
    apply_change(ch);
    return Pos{line, p.col}; // cursor stays where it was
}

Pos Buffer::erase_range(TextRange r) {
    if (r.end < r.start) {
        std::swap(r.start, r.end);
    }
    const std::uint32_t sl = clamp_line(lines_, r.start.line);
    const std::uint32_t el = clamp_line(lines_, r.end.line);

    const std::string& s_line = lines_[sl];
    const std::string& e_line = lines_[el];
    const std::size_t s_byte = cp_to_byte(s_line, r.start.col);
    const std::size_t e_byte = cp_to_byte(e_line, r.end.col);

    // Copy BEFORE state of every affected line.
    std::vector<std::string> before;
    for (std::uint32_t i = sl; i <= el; ++i) {
        before.push_back(lines_[i]);
    }

    // AFTER state: head of first line + tail of last line.
    std::string after = std::string{std::string_view{s_line}.substr(0, s_byte)};
    if (el != sl) {
        after += std::string{std::string_view{e_line}.substr(e_byte)};
    } else if (e_byte > s_byte) {
        after += std::string{std::string_view{s_line}.substr(e_byte)};
    }

    TextChange ch;
    ch.kind = EditKind::Bulk;
    ch.first_line = sl;
    ch.old_lines = std::move(before);
    ch.new_lines.push_back(std::move(after));
    ch.cur_before = Pos{sl, r.start.col};
    ch.cur_after = Pos{sl, r.start.col};
    apply_change(ch);
    return Pos{sl, r.start.col};
}

Pos Buffer::insert_newline(Pos p) {
    const std::uint32_t line = clamp_line(lines_, p.line);
    const std::string before = lines_[line];
    const std::size_t at = cp_to_byte(before, p.col);

    std::vector<std::string> after;
    after.push_back(std::string{std::string_view{before}.substr(0, at)});
    after.push_back(std::string{std::string_view{before}.substr(at)});

    TextChange ch;
    ch.kind = EditKind::Bulk;
    ch.first_line = line;
    ch.old_lines.push_back(before);
    ch.new_lines = std::move(after);
    ch.cur_before = Pos{line, p.col};
    ch.cur_after = Pos{line + 1, 0};
    apply_change(ch);
    return Pos{line + 1, 0};
}

TextRange Buffer::join(Pos p, bool smart) {
    const std::uint32_t line = clamp_line(lines_, p.line);
    if (line + 1 >= lines_.size()) {
        // Nothing below `p` to join: documented empty-range sentinel.
        return TextRange{Pos{line, 0}, Pos{line, 0}};
    }

    const std::string a_full = lines_[line];
    const std::string b_full = lines_[line + 1];
    const std::uint32_t a_full_len = line_len_cp(a_full);

    // Trailing-whitespace run of A = bytes after the last non-blank cp
    // (scanning forward but remembering the last non-blank end, NOT stopping
    // at the first internal blank).
    std::size_t a_keep = 0;
    if (smart) {
        std::size_t i = 0;
        std::size_t last_nonblank_end = 0;
        while (i < a_full.size()) {
            const auto d = decode_utf8(a_full, i);
            const std::size_t len = d ? d->second : std::size_t{1};
            if (!d || !is_blank(d->first)) {
                last_nonblank_end = i + len;
            }
            i += len;
        }
        a_keep = last_nonblank_end;
    } else {
        a_keep = a_full.size();
    }

    std::size_t b_skip = 0;
    if (smart) {
        std::size_t i = 0;
        while (i < b_full.size()) {
            const auto d = decode_utf8(b_full, i);
            const std::size_t len = d ? d->second : std::size_t{1};
            if (!d || !is_blank(d->first)) {
                break;
            }
            i += len;
        }
        b_skip = i;
    }

    std::string joined = a_full.substr(0, a_keep);
    const std::string b_tail = b_full.substr(b_skip);
    const bool need_space = smart && !joined.empty() && !b_tail.empty();
    if (need_space) {
        joined += ' ';
    }
    joined += b_tail;

    TextChange ch;
    ch.kind = EditKind::Bulk;
    ch.first_line = line;
    ch.old_lines.push_back(a_full);
    ch.old_lines.push_back(b_full);
    ch.new_lines.push_back(joined);
    ch.cur_before = Pos{line, a_full_len};
    apply_change(ch);

    // Collapsed position = where B's tail starts on the joined line. The range
    // covers the seam that was removed (empty range = nothing was removed).
    const std::uint32_t spot_cp = byte_to_cp(joined, a_keep);
    const TextRange r{Pos{line, spot_cp}, Pos{line, spot_cp}};
    return r;
}

std::shared_ptr<const std::vector<std::string>>
Buffer::snapshot_lines() {
    if (snapshot_cache_ != nullptr && snapshot_rev_ == revision_) {
        return snapshot_cache_;
    }
    auto fresh =
        std::make_shared<const std::vector<std::string>>(lines_);
    snapshot_cache_ = fresh;
    snapshot_rev_ = revision_;
    return fresh;
}

} // namespace noni
