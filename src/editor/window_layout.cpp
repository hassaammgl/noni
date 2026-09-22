#include <editor/window_layout.hpp>

void WindowLayout::clamp_ratio(float &r)
{
    r = std::clamp(r, 0.15f, 0.85f);
}

void WindowLayout::layout_node(
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

bool WindowLayout::split_leaf(LayoutNode *leaf, SplitOrientation orient)
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

void WindowLayout::reset(Buffer *buffer)
{
    root_ = LayoutNode::make_leaf(buffer);
    active_ = root_->leaf_window();
}

std::vector<Window *> WindowLayout::leaves()
{
    std::vector<Window *> out;
    if (root_)
        root_->collect_leaves(out);
    return out;
}

std::vector<Window *> WindowLayout::leaves() const
{
    std::vector<Window *> out;
    if (root_)
        const_cast<LayoutNode *>(root_.get())->collect_leaves(out);
    return out;
}

bool WindowLayout::split(SplitOrientation orient)
{
    if (!root_ || !active_)
        return false;
    LayoutNode *leaf = root_->find_leaf_node(active_);
    return split_leaf(leaf, orient);
}

bool WindowLayout::close_active()
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
