// P1.2 — see buffer.hpp.
#include "core/buffer.hpp"

#include <cassert>
#include <cstdio>
#include <string_view>
#include <utility>

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

} // namespace noni
