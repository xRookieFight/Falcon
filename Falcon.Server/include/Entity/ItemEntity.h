#pragma once

#include "Entity/Entity.h"
#include "Protocol/Types/ItemStack.h"

class ItemEntity : public Entity {
public:
    ItemEntity(uint64_t runtimeId, const ItemStack &item);

    const char *getIdentifier() const override { return "minecraft:item"; }

    const ItemStack &getItem() const { return mItem; }

    void setItem(const ItemStack &item) { mItem = item; }

    int getPickupDelay() const { return mPickupDelay; }

    void setPickupDelay(int pickupDelay) { mPickupDelay = pickupDelay; }

    bool canPickup() const { return mPickupDelay <= 0; }

    void tick();

private:
    ItemStack mItem;
    int mPickupDelay = 10;
};
