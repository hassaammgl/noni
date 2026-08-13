#include "ui/UIComponent.hpp"

UIComponent::~UIComponent()
{
    if (window)
    {
        delwin(window);
    }
}

void UIComponent::create(int height, int width, int y, int x)
{
    this->height = height;
    this->width = width;
    this->x = x;
    this->y = y;
}

void UIComponent::resize(
    int height,
    int width,
    int y,
    int x)
{
    if (window)
    {
        delwin(window);
        window = nullptr;
    }

    this->height = height;
    this->width = width;
    this->y = y;
    this->x = x;

    window = newwin(
        height,
        width,
        y,
        x);
}

void UIComponent::clear()
{
    if (window)
    {
        werase(window);
    }
}

WINDOW *UIComponent::getWindow() const
{
    return window;
}
