#pragma once

#include "block/BlockActor.h"
#include "block/inventory/FurnaceInventory.h"
#include "protocol/types/ItemStack.h"

#include <array>

class FurnaceBlockActor final : public BlockActor {
public:
    std::array<ItemStack, FurnaceInventory::SIZE> mItems;
    FurnaceKind mKind = FurnaceKind::Furnace;
    int mBurnTime = 0;
    int mMaxBurnTime = 0;
    int mCookTime = 0;
};
