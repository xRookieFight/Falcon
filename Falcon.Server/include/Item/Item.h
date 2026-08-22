#pragma once

#include "Block/Block.h"

#include <cstdint>
#include <string>

class Item {
public:
    Item() : mTypeId(0), mIdentifier("minecraft:air"), mName("Air"), mMaxStackSize(64), mIsBlock(false) {}

    Item(int32_t typeId, const std::string &identifier, const std::string &name, int32_t maxStackSize = 64)
            : mTypeId(typeId), mIdentifier(identifier), mName(name), mMaxStackSize(maxStackSize), mIsBlock(false) {}

    Item(const Block &block, int32_t maxStackSize = 64)
            : mTypeId(block.getTypeId()), mIdentifier(block.getIdentifier()), mName(block.getName()),
              mMaxStackSize(maxStackSize), mIsBlock(true), mBlock(block) {}

    int32_t getTypeId() const { return mTypeId; }

    const std::string &getIdentifier() const { return mIdentifier; }

    const std::string &getName() const { return mName; }

    int32_t getMaxStackSize() const { return mMaxStackSize; }

    bool isBlock() const { return mIsBlock; }

    const Block &getBlock() const { return mBlock; }

private:
    int32_t mTypeId;
    std::string mIdentifier;
    std::string mName;
    int32_t mMaxStackSize;
    bool mIsBlock;
    Block mBlock;
};
