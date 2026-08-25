#pragma once

#include "Protocol/Types/ItemStack.h"

class Container {
public:
    virtual ~Container() = default;

    virtual int getContainerSize() const = 0;

    virtual const ItemStack &getContainerItem(int slot) const = 0;

    virtual void setContainerItem(int slot, ItemStack item) = 0;
};
