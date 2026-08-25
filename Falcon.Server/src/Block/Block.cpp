#include "block/Block.h"

#include "block/components/BlockBehavior.h"
#include "block/components/BlockBehaviorRegistry.h"
#include "block/BlockData.h"

const BlockData *Block::getData() const {
    return BlockDataTable::find(mIdentifier.c_str());
}

const BlockBehavior &Block::getBehavior() const {
    const BlockData *data = getData();
    if (data != nullptr && data->mBehaviorIdentifier != nullptr)
        return BlockBehaviorRegistry::get(data->mBehaviorIdentifier);
    return BlockBehaviorRegistry::get(mIdentifier);
}

float Block::getFrictionFactor() const {
    return getBehavior().getFrictionFactor();
}

bool Block::onEntityLand(Actor &actor, float downwardVelocity) const {
    return getBehavior().onEntityLand(actor, downwardVelocity);
}

std::optional<float> Block::getFallDamage(const Actor &actor, float vanillaFallDamage) const {
    return getBehavior().getFallDamage(actor, vanillaFallDamage);
}
