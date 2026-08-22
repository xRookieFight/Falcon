#include "Entity/LivingEntity.h"

#include <cmath>

namespace {
    const char *ATTRIBUTE_HEALTH = "minecraft:health";
}

const float LivingEntity::FALL_DAMAGE_THRESHOLD = 3.0f;

void LivingEntity::kill() {
    mIsDead = true;
    getAttributes().set(ATTRIBUTE_HEALTH, 0.0f);
    setMotion(Vector3f(0.0f, 0.0f, 0.0f));
    resetFallDistance();
}

float LivingEntity::reduceHealth(float amount) {
    const float newHealth = getHealth() - amount;
    getAttributes().set(ATTRIBUTE_HEALTH, newHealth > 0.0f ? newHealth : 0.0f);
    return newHealth;
}

float LivingEntity::computeFallDamage() const {
    const float distance = getFallDistance();
    if (distance <= FALL_DAMAGE_THRESHOLD)
        return 0.0f;

    const float damage = std::floor(distance - FALL_DAMAGE_THRESHOLD);
    return damage < 1.0f ? 0.0f : damage;
}
