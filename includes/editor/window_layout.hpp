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

    static void clamp_ratio(float &r);
    static void layout_node(
        const LayoutNode *node,
        int x,
        int y,
        int w,
        int h,
        std::vector<std::pair<Window *, WindowRect>> &out);
    bool split_leaf(LayoutNode *leaf, SplitOrientation orient);

public:
    void reset(Buffer *buffer);
    LayoutNode *root() { return root_.get(); }
    const LayoutNode *root() const { return root_.get(); }
    Window *active() { return active_; }
    const Window *active() const { return active_; }
    void set_active(Window *w)
    {
        if (w)
            active_ = w;
    }
    std::vector<Window *> leaves();
    std::vector<Window *> leaves() const;
    bool split(SplitOrientation orient);
    bool split_vertical() { return split(SplitOrientation::Vertical); }
    bool split_horizontal() { return split(SplitOrientation::Horizontal); }
    bool close_active();
    void resize_active(SplitOrientation along, float delta);
    bool focus_neighbor(int dx, int dy, int area_w, int area_h);
    std::vector<std::pair<Window *, WindowRect>> compute_rects(int x, int y, int w, int h) const;
};
