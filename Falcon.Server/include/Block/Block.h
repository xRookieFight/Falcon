#pragma once

#include "Core/NBT/Tag.h"
#include "Block/BlockState.h"

#include <cstdint>
#include <optional>
#include <string>

struct BlockData;
class Actor;
class BlockBehavior;

class Block {
public:
    Block() : mTypeId(0), mIdentifier("minecraft:air"), mName("Air"), mStates(Tag::ofCompound()) {}

    Block(int32_t typeId, const std::string &identifier, const std::string &name)
            : mTypeId(typeId), mIdentifier(identifier), mName(name), mStates(Tag::ofCompound()) {}

    Block(int32_t typeId, const std::string &identifier, const std::string &name, const Tag &states)
            : mTypeId(typeId), mIdentifier(identifier), mName(name), mStates(states) {}

    explicit Block(const BlockState &state)
            : mTypeId(0), mIdentifier(state.mName), mName(state.mName), mStates(state.mStates) {}

    int32_t getTypeId() const { return mTypeId; }

    const std::string &getIdentifier() const { return mIdentifier; }

    const std::string &getName() const { return mName; }

    bool isFire() const {
        return mIdentifier == "minecraft:fire" || mIdentifier == "minecraft:soul_fire";
    }

    const Tag &getStates() const { return mStates; }

    const BlockData *getData() const;

    const BlockBehavior &getBehavior() const;

    float getFrictionFactor() const;

    bool onEntityLand(Actor &actor, float downwardVelocity) const;

    std::optional<float> getFallDamage(const Actor &actor, float vanillaFallDamage) const;

    BlockState toBlockState() const { return BlockState(mIdentifier, mStates); }

    int32_t getNetworkHash() const { return BlockStateHasher::hash(mIdentifier, mStates); }

private:
    int32_t mTypeId;
    std::string mIdentifier;
    std::string mName;
    Tag mStates;
};
