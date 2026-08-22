#pragma once

#include "Entity/Entity.h"

class LivingEntity : public Entity {
public:
    explicit LivingEntity(uint64_t runtimeId) : Entity(runtimeId) {}

    bool isDead() const { return mIsDead; }

    void setDead(bool isDead) { mIsDead = isDead; }

    void kill();

    float reduceHealth(float amount);

    float computeFallDamage() const;

private:
    static const float FALL_DAMAGE_THRESHOLD;

    bool mIsDead = false;
};
