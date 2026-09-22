#include <editor/window_layout.hpp>

void WindowLayout::resize_active(SplitOrientation along, float delta)
{
    if (!root_ || !active_)
        return;
    LayoutNode *node = root_->find_leaf_node(active_);
    while (node)
    {
        LayoutNode *parent = root_->find_parent_of(node);
        if (!parent)
            break;
        if (parent->orientation == along)
        {
            if (parent->first.get() == node)
                parent->ratio += delta;
            else
                parent->ratio -= delta;
            clamp_ratio(parent->ratio);
            return;
        }
        node = parent;
    }
}

bool WindowLayout::focus_neighbor(int dx, int dy, int area_w, int area_h)
{
    auto rects = compute_rects(0, 0, area_w, area_h);
    if (!active_)
        return false;

    WindowRect cur_r{};
    bool found = false;
    for (auto &[w, r] : rects)
    {
        if (w == active_)
        {
            cur_r = r;
            found = true;
            break;
        }
    }
    if (!found)
        return false;

    const int cx = cur_r.x + cur_r.width / 2;
    const int cy = cur_r.y + cur_r.height / 2;
    Window *best = nullptr;
    int best_dist = 1 << 30;

    for (auto &[w, r] : rects)
    {
        if (w == active_)
            continue;
        const int wx = r.x + r.width / 2;
        const int wy = r.y + r.height / 2;
        const int ddx = wx - cx;
        const int ddy = wy - cy;
        if (dx != 0 && ddx * dx <= 0)
            continue;
        if (dy != 0 && ddy * dy <= 0)
            continue;
        if (dx != 0 && std::abs(ddy) > std::abs(ddx) * 2)
            continue;
        if (dy != 0 && std::abs(ddx) > std::abs(ddy) * 2)
            continue;
        const int dist = std::abs(ddx) + std::abs(ddy);
        if (dist < best_dist)
        {
            best_dist = dist;
            best = w;
        }
    }
    if (!best)
        return false;
    active_ = best;
    return true;
}

std::vector<std::pair<Window *, WindowRect>> WindowLayout::compute_rects(int x, int y, int w, int h) const
{
    std::vector<std::pair<Window *, WindowRect>> out;
    if (root_ && w > 0 && h > 0)
        layout_node(root_.get(), x, y, w, h, out);
    return out;
}
