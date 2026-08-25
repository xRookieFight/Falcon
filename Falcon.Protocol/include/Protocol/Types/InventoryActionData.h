#pragma once

#include "protocol/types/InventorySource.h"
#include "protocol/types/ItemStack.h"

#include <cstdint>

class InventoryActionData {
public:
    InventorySource mSource;
    int32_t mSlot = 0;
    ItemStack mFromItem;
    ItemStack mToItem;
};
