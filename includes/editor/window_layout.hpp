#pragma once

#include <editor/window.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

enum class SplitOrientation
{
    Horizontal, // stacked top/bottom
    Vertical,   // side by side
};

struct WindowRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

class LayoutNode
{
public:
    enum class Kind
    {
        Leaf,
        Split,
    };

    Kind kind = Kind::Leaf;
    SplitOrientation orientation = SplitOrientation::Vertical;
    float ratio = 0.5f;
    std::unique_ptr<Window> window;
    std::unique_ptr<LayoutNode> first;
    std::unique_ptr<LayoutNode> second;

    static std::unique_ptr<LayoutNode> make_leaf(Buffer *buffer)
    {
        auto node = std::make_unique<LayoutNode>();
        node->kind = Kind::Leaf;
        node->window = std::make_unique<Window>(buffer);
        return node;
    }

    bool is_leaf() const { return kind == Kind::Leaf; }

    Window *leaf_window() { return is_leaf() ? window.get() : nullptr; }
    const Window *leaf_window() const { return is_leaf() ? window.get() : nullptr; }

    void collect_leaves(std::vector<Window *> &out)
    {
        if (is_leaf())
        {
            if (window)
                out.push_back(window.get());
            return;
        }
        if (first)
            first->collect_leaves(out);
        if (second)
            second->collect_leaves(out);
    }

    LayoutNode *find_parent_of(LayoutNode *child)
    {
        if (is_leaf())
            return nullptr;
        if (first.get() == child || second.get() == child)
            return this;
        if (LayoutNode *p = first ? first->find_parent_of(child) : nullptr)
            return p;
        return second ? second->find_parent_of(child) : nullptr;
    }

    LayoutNode *find_leaf_node(Window *w)
    {
        if (is_leaf())
            return (window.get() == w) ? this : nullptr;
        if (LayoutNode *n = first ? first->find_leaf_node(w) : nullptr)
            return n;
        return second ? second->find_leaf_node(w) : nullptr;
    }
};

class WindowLayout
{
private:
    std::unique_ptr<LayoutNode> root_;
    Window *active_ = nullptr;

    static void clamp_ratio(float &r)
    {
        r = std::clamp(r, 0.15f, 0.85f);
    }

    static void layout_node(
        const LayoutNode *node,
        int x,
        int y,
        int w,
        int h,
        std::vector<std::pair<Window *, WindowRect>> &out)
    {
        if (!node || w <= 0 || h <= 0)
            return;

        if (node->is_leaf())
        {
            if (node->window)
                out.push_back({node->window.get(), WindowRect{x, y, w, h}});
            return;
        }

        float ratio = node->ratio;
        clamp_ratio(ratio);

        if (node->orientation == SplitOrientation::Vertical)
        {
            const int sep = 1;
            int left_w = std::max(1, static_cast<int>(std::lround(w * ratio)));
            left_w = std::min(left_w, std::max(1, w - sep - 1));
            const int right_w = std::max(1, w - left_w - sep);
            layout_node(node->first.get(), x, y, left_w, h, out);
            layout_node(node->second.get(), x + left_w + sep, y, right_w, h, out);
        }
        else
        {
            const int sep = 1;
            int top_h = std::max(1, static_cast<int>(std::lround(h * ratio)));
            top_h = std::min(top_h, std::max(1, h - sep - 1));
            const int bot_h = std::max(1, h - top_h - sep);
            layout_node(node->first.get(), x, y, w, top_h, out);
            layout_node(node->second.get(), x, y + top_h + sep, w, bot_h, out);
        }
    }

    bool split_leaf(LayoutNode *leaf, SplitOrientation orient)
    {
        if (!leaf || !leaf->is_leaf() || !leaf->window)
            return false;

        Buffer *buf = leaf->window->has_buffer() ? &leaf->window->buffer() : nullptr;

        auto keep = std::make_unique<LayoutNode>();
        keep->kind = LayoutNode::Kind::Leaf;
        keep->window = std::move(leaf->window);

        auto other = LayoutNode::make_leaf(buf);
        if (keep->window)
        {
            other->window->cursor() = keep->window->cursor();
            other->window->scroll_y() = keep->window->scroll_y();
            other->window->scroll_x() = keep->window->scroll_x();
        }

        leaf->kind = LayoutNode::Kind::Split;
        leaf->orientation = orient;
        leaf->ratio = 0.5f;
        leaf->window.reset();
        leaf->first = std::move(keep);
        leaf->second = std::move(other);
        active_ = leaf->second->leaf_window();
        return true;
    }

public:
    void reset(Buffer *buffer)
    {
        root_ = LayoutNode::make_leaf(buffer);
        active_ = root_->leaf_window();
    }

    LayoutNode *root() { return root_.get(); }
    const LayoutNode *root() const { return root_.get(); }

    Window *active() { return active_; }
    const Window *active() const { return active_; }
    void set_active(Window *w)
    {
        if (w)
            active_ = w;
    }

    std::vector<Window *> leaves()
    {
        std::vector<Window *> out;
        if (root_)
            root_->collect_leaves(out);
        return out;
    }

    std::vector<Window *> leaves() const
    {
        std::vector<Window *> out;
        if (root_)
            const_cast<LayoutNode *>(root_.get())->collect_leaves(out);
        return out;
    }

    bool split(SplitOrientation orient)
    {
        if (!root_ || !active_)
            return false;
        LayoutNode *leaf = root_->find_leaf_node(active_);
        return split_leaf(leaf, orient);
    }

    bool split_vertical() { return split(SplitOrientation::Vertical); }
    bool split_horizontal() { return split(SplitOrientation::Horizontal); }

    // Close active window without destroying its Buffer.
    bool close_active()
    {
        if (!root_ || !active_)
            return false;
        if (leaves().size() <= 1)
            return false;

        LayoutNode *leaf = root_->find_leaf_node(active_);
        if (!leaf)
            return false;

        LayoutNode *parent = root_->find_parent_of(leaf);
        if (!parent || parent->is_leaf())
            return false;

        std::unique_ptr<LayoutNode> sibling =
            (parent->first.get() == leaf) ? std::move(parent->second) : std::move(parent->first);

        if (parent == root_.get())
        {
            root_ = std::move(sibling);
        }
        else
        {
            LayoutNode *grand = root_->find_parent_of(parent);
            if (!grand)
                return false;
            if (grand->first.get() == parent)
                grand->first = std::move(sibling);
            else
                grand->second = std::move(sibling);
        }

        auto rem = leaves();
        active_ = rem.empty() ? nullptr : rem.front();
        return true;
    }

    void resize_active(SplitOrientation along, float delta)
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

    bool focus_neighbor(int dx, int dy, int area_w, int area_h)
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

    std::vector<std::pair<Window *, WindowRect>> compute_rects(int x, int y, int w, int h) const
    {
        std::vector<std::pair<Window *, WindowRect>> out;
        if (root_ && w > 0 && h > 0)
            layout_node(root_.get(), x, y, w, h, out);
        return out;
    }
};
