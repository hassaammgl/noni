#pragma once

#include <ncurses.h>

class UIComponent
{
protected:
    WINDOW *window = nullptr;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool dirty_ = true;

public:
    UIComponent() = default;
    virtual ~UIComponent();

    virtual void create(int height, int width, int y, int x);

    virtual void resize(int height, int width, int y, int x);

    virtual void draw() = 0;

    virtual void clear();

    WINDOW *get_window() const;

    void mark_dirty() { dirty_ = true; }
    void clear_dirty() { dirty_ = false; }
    [[nodiscard]] bool is_dirty() const { return dirty_; }

    // draw() only when dirty; returns true if painted.
    bool paint_if_dirty()
    {
        if (!dirty_)
            return false;
        draw();
        dirty_ = false;
        return true;
    }
};
