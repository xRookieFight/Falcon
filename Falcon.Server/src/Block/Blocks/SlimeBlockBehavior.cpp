#include "Block/Blocks/SlimeBlockBehavior.h"

#include "Actor/Actor.h"
#include "Actor/ActorFlags.h"

bool SlimeBlockBehavior::onEntityLand(Actor &actor, float downwardVelocity) const {
    if (actor.getFlags().get(ActorFlag::Sneaking))
        return false;

    Vector3f motion = actor.getMotion();
    motion.y = -downwardVelocity;
    actor.setMotion(motion);
    actor.resetFallDistance();
    return true;
}

std::optional<float> SlimeBlockBehavior::getFallDamage(const Actor &actor, float vanillaFallDamage) const {
    (void) vanillaFallDamage;
    if (actor.getFlags().get(ActorFlag::Sneaking))
        return std::nullopt;
    return 0.0f;
}
