// P3.6 — see view.hpp.
#include "core/view.hpp"

#include <algorithm>

namespace noni {

namespace {

[[nodiscard]] std::uint32_t last_line(const ViewGeometry& g) {
    return g.line_count == 0 ? 0 : g.line_count - 1;
}

[[nodiscard]] int height(const ViewGeometry& g) {
    return std::max(1, g.rows);
}

// vim 'scroll' default: half the window height (11 for a 22-row window).
[[nodiscard]] int half_page(const ViewGeometry& g) {
    return std::max(1, height(g) / 2);
}

// vim CTRL-F/CTRL-B page: window height minus two overlap lines.
[[nodiscard]] int page_size(const ViewGeometry& g) {
    return std::max(1, height(g) - 2);
}

// Effective 'scrolloff': never more than half the view (vim clamps it).
[[nodiscard]] int effective_scrolloff(const ViewGeometry& g) {
    return std::clamp(g.scrolloff, 0, std::max(0, (height(g) - 1) / 2));
}

[[nodiscard]] std::uint32_t clamp_top(const ViewGeometry& g, long long top) {
    return static_cast<std::uint32_t>(
        std::clamp<long long>(top, 0, static_cast<long long>(last_line(g))));
}

} // namespace

std::uint32_t visible_rows(const ViewGeometry& g, const std::uint32_t top) {
    const std::uint32_t last = last_line(g);
    const std::uint32_t t = std::min(top, last);
    const std::uint32_t in_buffer = last - t + 1;
    return std::min<std::uint32_t>(in_buffer,
                                   static_cast<std::uint32_t>(height(g)));
}

std::uint32_t follow_top(const ViewGeometry& g,
                         const std::uint32_t cursor_line,
                         const std::uint32_t top) {
    const std::uint32_t last = last_line(g);
    const std::uint32_t cur = std::min(cursor_line, last);
    const auto rows = static_cast<std::uint32_t>(height(g));
    const auto so = static_cast<std::uint32_t>(effective_scrolloff(g));
    std::uint32_t t = std::min(top, last);
    if (cur < t + so) {
        t = cur > so ? cur - so : 0;
    } else if (cur + so >= t + rows) {
        // Cursor sits closer than `scrolloff` to the bottom edge (or below).
        t = cur + so - rows + 1;
    }
    return std::min(t, last);
}

ScrollResult apply_scroll(const ViewGeometry& g, const ScrollRequest& r) {
    const std::uint32_t last = last_line(g);
    const std::uint32_t cur = std::min(r.cursor_line, last);
    const std::uint32_t top_in = std::min(r.top, last);
    const auto rows = static_cast<std::uint32_t>(height(g));
    const std::uint32_t count = std::max<std::uint32_t>(1, r.count);

    // z-command target: an explicit [count] is a 1-based line, otherwise the
    // cursor's own line.
    const std::uint32_t target =
        r.counted ? std::min(last, count - 1) : cur;

    // A counted z-command moves the cursor onto the target line (vim does
    // that when the line would otherwise be scrolled away); plain zz/zt/zb
    // leave the cursor alone.
    const auto with_target_cursor = [&](const std::uint32_t new_top) {
        return ScrollResult{r.counted ? target : cur, new_top};
    };

    switch (r.id) {
    case ScrollId::PageDown: {
        // Oracle quirk: when the LAST line is already on screen, C-f does not
        // scroll into '~' lanes by a full page — it puts the last line at the
        // top (cursor on it) instead.
        if (top_in + rows >= g.line_count) {
            return ScrollResult{last, last};
        }
        const auto step = static_cast<long long>(count)
                        * static_cast<long long>(page_size(g));
        const std::uint32_t nt =
            clamp_top(g, static_cast<long long>(top_in) + step);
        return ScrollResult{nt, nt}; // cursor to the first line of the page
    }
    case ScrollId::PageUp: {
        const auto step = static_cast<long long>(count)
                        * static_cast<long long>(page_size(g));
        const std::uint32_t nt =
            clamp_top(g, static_cast<long long>(top_in) - step);
        if (nt == top_in) {
            return ScrollResult{cur, top_in}; // already at the top: no move
        }
        return ScrollResult{std::min(last, nt + rows - 1), nt};
    }
    case ScrollId::HalfDown: {
        // vim never scrolls C-d past the last full page (the last line stays
        // at the bottom): that is what makes C-d clamp at EOF. A [count] is
        // an exact line count; without one vim scrolls half a page.
        const auto max_top = std::max<long long>(
            0, static_cast<long long>(g.line_count) - static_cast<long long>(rows));
        const auto step = static_cast<long long>(
            r.counted ? count : static_cast<std::uint32_t>(half_page(g)));
        const auto nt = std::clamp<long long>(
            static_cast<long long>(top_in) + step, 0, max_top);
        const std::uint32_t moved =
            static_cast<std::uint32_t>(nt);
        std::uint32_t nc = static_cast<std::uint32_t>(std::min<long long>(
            static_cast<long long>(cur) + step, static_cast<long long>(last)));
        nc = std::clamp(nc, moved, std::min(last, moved + rows - 1));
        return ScrollResult{nc, moved};
    }
    case ScrollId::HalfUp: {
        const auto step = static_cast<long long>(
            r.counted ? count : static_cast<std::uint32_t>(half_page(g)));
        const auto nt = std::max<long long>(
            0, static_cast<long long>(top_in) - step);
        const std::uint32_t moved = static_cast<std::uint32_t>(nt);
        const std::uint32_t nc = static_cast<std::uint32_t>(std::max<long long>(
            0, static_cast<long long>(cur) - step));
        return ScrollResult{std::clamp(nc, moved, std::min(last, moved + rows - 1)),
                            moved};
    }
    case ScrollId::LineTop: {
        const std::uint32_t vis = visible_rows(g, top_in);
        return ScrollResult{top_in + std::min<std::uint32_t>(count - 1, vis - 1),
                            top_in};
    }
    case ScrollId::LineMiddle: {
        const std::uint32_t vis = visible_rows(g, top_in);
        return ScrollResult{top_in + (vis - 1) / 2, top_in};
    }
    case ScrollId::LineBottom: {
        const std::uint32_t vis = visible_rows(g, top_in);
        const std::uint32_t bottom = top_in + vis - 1;
        const std::uint32_t back = std::min<std::uint32_t>(count - 1, vis - 1);
        return ScrollResult{bottom - back, top_in};
    }
    case ScrollId::Center: {
        // Center the target in the WINDOW (rows, not the visible text).
        return with_target_cursor(
            clamp_top(g, static_cast<long long>(target)
                             - static_cast<long long>((rows - 1) / 2)));
    }
    case ScrollId::Top:
        return with_target_cursor(target);
    case ScrollId::Bottom:
        return with_target_cursor(clamp_top(
            g, static_cast<long long>(target) - static_cast<long long>(rows - 1)));
    }
    return ScrollResult{cur, top_in};
}

} // namespace noni
