#pragma once

#include "Block/BlockActor.h"
#include "Block/Inventory/FurnaceInventory.h"
#include "Protocol/Types/ItemStack.h"

#include <array>

class FurnaceBlockActor final : public BlockActor {
public:
    std::array<ItemStack, FurnaceInventory::SIZE> mItems;
    FurnaceKind mKind = FurnaceKind::Furnace;
    int mBurnTime = 0;
    int mMaxBurnTime = 0;
    int mCookTime = 0;
};
