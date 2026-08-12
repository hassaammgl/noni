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

public:
    UIComponent() = default;

    virtual ~UIComponent();

    virtual void create(
        int height,
        int width,
        int y,
        int x);

    virtual void resize(
        int height,
        int width,
        int y,
        int x);

    virtual void draw() = 0;

    virtual void clear();

    WINDOW *getWindow() const;
};