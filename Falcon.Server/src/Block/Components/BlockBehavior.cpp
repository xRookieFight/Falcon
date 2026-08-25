#include "block/components/BlockBehavior.h"

#include "actor/Actor.h"

bool BlockBehavior::onEntityLand(Actor &actor, float downwardVelocity) const {
    (void) actor;
    (void) downwardVelocity;
    return false;
}

std::optional<float> BlockBehavior::getFallDamage(const Actor &actor, float vanillaFallDamage) const {
    (void) actor;
    (void) vanillaFallDamage;
    return std::nullopt;
}
