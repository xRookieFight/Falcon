#pragma once

#include "core/nbt/Tag.h"
#include "protocol/types/BlockDefinition.h"
#include "protocol/types/ItemDefinition.h"

#include <memory>
#include <string>
#include <vector>

class ItemStack {
public:
    static ItemStack air();

    bool isAir() const { return mDefinition == nullptr; }

    std::shared_ptr<ItemDefinition> mDefinition;
    std::shared_ptr<BlockDefinition> mBlockDefinition;
    int mCount = 0;
    int mDamage = 0;
    Tag mTag;
    std::vector<std::string> mCanPlace;
    std::vector<std::string> mCanBreak;
    int64_t mBlockingTicks = 0;
    bool mUsingNetId = false;
    int32_t mNetId = 0;
};
