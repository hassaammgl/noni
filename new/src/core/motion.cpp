// P2.7 — see motion.hpp.
#include "core/motion.hpp"

#include <algorithm>

namespace noni {

Pos Motion::apply(const MotionContext& ctx, const Pos from,
                  const std::uint32_t count, const std::uint32_t desired) const {
    if (ctx.lines == nullptr || ctx.lines->empty()) {
        return from;
    }
    const auto& lines = *ctx.lines;
    // Clamps `to` onto [0, last-char] for its LINE (empty line -> col 0).
    const auto clamp_norm = [&](const Pos p) {
        const std::uint32_t len =
            static_cast<std::uint32_t>(line_len_cp((*ctx.lines)[p.line]));
        if (len == 0) {
            return Pos{p.line, 0};
        }
        return Pos{p.line, std::min(p.col, len - 1)};
    };

    switch (id) {
    case MotionId::H: {
        const std::uint32_t back = std::min(from.col, count);
        return clamp_norm(Pos{from.line, from.col - back});
    }
    case MotionId::L: {
        const std::uint32_t len =
            static_cast<std::uint32_t>(line_len_cp(lines[from.line]));
        const std::uint32_t last = len == 0 ? 0 : len - 1;
        return Pos{from.line,
                   static_cast<std::uint32_t>(std::min<std::uint64_t>(
                       static_cast<std::uint64_t>(from.col) + count,
                       static_cast<std::uint64_t>(last)))};
    }
    case MotionId::J:
    case MotionId::K: {
        // Oracle (j_col_clamp_persists_via_k_back): the col clamp does NOT
        // erase the aim — `desired` is remembered across shorter lines and
        // the NEXT vertical move recovers it (clamped again).
        const bool down = id == MotionId::J;
        const std::int64_t target = down
            ? static_cast<std::int64_t>(from.line) + count
            : static_cast<std::int64_t>(from.line) - count;
        const std::uint32_t nl = static_cast<std::uint32_t>(std::clamp(
            target, static_cast<std::int64_t>(0),
            static_cast<std::int64_t>(lines.size() - 1)));
        return clamp_norm(Pos{nl, desired});
    }
    case MotionId::Zero:
        return Pos{from.line, 0};
    case MotionId::Dollar: {
        // vim N$: end of line (from.line + count - 1), clamped to the last
        // line. Oracle quirk (tests/diff probes n2..n8): when the START is
        // already the last line (no downward move is possible at all), the
        // whole command FAILS — cursor stays exactly where it was (even a
        // mid-line col stays).
        const std::size_t n = lines.size();
        if (count <= 1) {
            const std::uint32_t len = static_cast<std::uint32_t>(
                line_len_cp(lines[from.line]));
            return Pos{from.line, len == 0 ? 0 : len - 1};
        }
        std::uint64_t target = static_cast<std::uint64_t>(from.line)
                             + static_cast<std::uint64_t>(count) - 1;
        if (target > n - 1) {
            target = n - 1;
        }
        if (target == from.line) {
            return from; // oracle: overflow with no downward move rejects
        }
        const std::uint32_t len = static_cast<std::uint32_t>(
            line_len_cp(lines[static_cast<std::size_t>(target)]));
        return Pos{static_cast<std::uint32_t>(target),
                   len == 0 ? 0 : len - 1};
    }
    }
    return from;
}

} // namespace noni
