#pragma once

#include "Core/NBT/Tag.h"
#include "Level/BlockState.h"

#include <cstdint>
#include <string>

class Block {
public:
    Block() : mTypeId(0), mIdentifier("minecraft:air"), mName("Air"), mStates(Tag::ofCompound()) {}

    Block(int32_t typeId, const std::string &identifier, const std::string &name)
            : mTypeId(typeId), mIdentifier(identifier), mName(name), mStates(Tag::ofCompound()) {}

    Block(int32_t typeId, const std::string &identifier, const std::string &name, const Tag &states)
            : mTypeId(typeId), mIdentifier(identifier), mName(name), mStates(states) {}

    int32_t getTypeId() const { return mTypeId; }

    const std::string &getIdentifier() const { return mIdentifier; }

    const std::string &getName() const { return mName; }

    const Tag &getStates() const { return mStates; }

    BlockState toBlockState() const { return BlockState(mIdentifier, mStates); }

    int32_t getNetworkHash() const { return BlockStateHasher::hash(mIdentifier, mStates); }

private:
    int32_t mTypeId;
    std::string mIdentifier;
    std::string mName;
    Tag mStates;
};
