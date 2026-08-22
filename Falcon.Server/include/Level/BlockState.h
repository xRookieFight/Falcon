#pragma once

#include "Core/NBT/Tag.h"
#include "Protocol/BlockStateHasher.h"

#include <cstdint>
#include <string>

class BlockState {
public:
    BlockState() : mName("minecraft:air"), mStates(Tag::ofCompound()), mHash(BlockStateHasher::hash("minecraft:air")) {}

    explicit BlockState(const std::string &name)
            : mName(name), mStates(Tag::ofCompound()), mHash(BlockStateHasher::hash(name)) {}

    BlockState(const std::string &name, const Tag &states)
            : mName(name), mStates(states), mHash(BlockStateHasher::hash(name, states)) {}

    bool operator==(const BlockState &other) const {
        return mHash == other.mHash && mName == other.mName;
    }

    Tag toNbt() const {
        Tag tag = Tag::ofCompound();
        tag.putString("name", mName);
        tag.put("states", mStates);
        tag.putInt("version", BLOCK_STATE_VERSION);
        return tag;
    }

    static const int32_t BLOCK_STATE_VERSION = 18163713;

    std::string mName;
    Tag mStates;
    int32_t mHash;
};
