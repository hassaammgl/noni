#pragma once

#include <editor/buffer_manager.hpp>
#include <ui/UIComponent.hpp>

class TabBar : public UIComponent
{
private:
    const BufferManager *manager = nullptr;

public:
    void set_manager(const BufferManager *manager);

    void draw() override;
};
