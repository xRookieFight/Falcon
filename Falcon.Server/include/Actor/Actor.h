#pragma once

#include "Actor/ActorAttributes.h"
#include "Actor/ActorFlags.h"
#include "Actor/MobEffect.h"
#include "Actor/ExperienceManager.h"
#include "Core/Math/Vector3f.h"

#include <cstdint>
#include <string>

class Actor {
public:
    explicit Actor(uint64_t runtimeId);

    virtual ~Actor() = default;

    virtual const char *getIdentifier() const = 0;

    uint64_t getRuntimeId() const { return mRuntimeId; }

    int64_t getUniqueId() const { return (int64_t) mRuntimeId; }

    const Vector3f &getPosition() const { return mPosition; }

    void setPosition(const Vector3f &position) { mPosition = position; }

    const Vector3f &getRotation() const { return mRotation; }

    void setRotation(const Vector3f &rotation) { mRotation = rotation; }

    const Vector3f &getMotion() const { return mMotion; }

    void setMotion(const Vector3f &motion) { mMotion = motion; }

    ActorFlags &getFlags() { return mFlags; }

    const ActorFlags &getFlags() const { return mFlags; }

    ActorAttributes &getAttributes() { return mAttributes; }

    const ActorAttributes &getAttributes() const { return mAttributes; }

    bool isOnGround() const { return mOnGround; }

    void setOnGround(bool onGround) { mOnGround = onGround; }

    float getFallDistance() const { return mFallDistance; }

    float getHighestPosition() const { return mHighestPosition; }

    void setHighestPosition(float highestPosition) { mHighestPosition = highestPosition; }

    void updateFallDistance() { mFallDistance = mHighestPosition - mPosition.y; }

    void resetFallDistance() {
        mFallDistance = 0.0f;
        mHighestPosition = mPosition.y;
    }

    ExperienceManager &getExperience() { return mExperience; }

    const ExperienceManager &getExperience() const { return mExperience; }

    void syncExperience() { mExperience.applyTo(mAttributes); }

    void setXpAndProgress(int level, float progress);

    void addXp(int amount);

    void addXpLevels(int amount);

    float getHealth() const;

    float getMaxHealth() const;

    bool isAlive() const;

    float getFood() const;

    float getMaxFood() const;

    void setFood(float food);

    void addFood(float amount);

    bool isHungry() const;

    bool canEat() const;

    float getSaturation() const;

    void setSaturation(float saturation);

    void addSaturation(float amount);

    float getExhaustion() const;

    void setExhaustion(float exhaustion);

    void exhaust(float amount);

    int getFoodTickTimer() const { return mFoodTickTimer; }

    void setFoodTickTimer(int foodTickTimer);

    bool isHungerEnabled() const { return mHungerEnabled; }

    void setHungerEnabled(bool enabled) { mHungerEnabled = enabled; }

    void consumeFood(int nutrition, float saturation);

    void resetHungerAndExperience();

    bool tickHunger(int tickDiff, int difficulty);

    bool isDead() const { return mIsDead; }

    void setDead(bool isDead) { mIsDead = isDead; }

    void kill();

    float reduceHealth(float amount);

    float heal(float amount);

    MobEffectManager &getEffects() { return mEffects; }

    const MobEffectManager &getEffects() const { return mEffects; }

    bool addEffect(const MobEffectInstance &effect) { return mEffects.add(effect); }

    bool removeEffect(MobEffectId id) { return mEffects.remove(id); }

    bool hasEffect(MobEffectId id) const { return mEffects.has(id); }

    const MobEffectInstance *getEffect(MobEffectId id) const { return mEffects.get(id); }

    bool tickEffects(int32_t tickDiff = 1) { return mEffects.tick(tickDiff); }

    float getMovementSpeedMultiplier() const { return mEffects.movementSpeedMultiplier(); }

    float getJumpVelocityMultiplier() const { return mEffects.jumpVelocityMultiplier(); }

    float computeFallDamage() const;

protected:
    static const float EXHAUSTION_PER_UNIT;
    static const float FALL_DAMAGE_THRESHOLD;
    static const int FOOD_TICK_PERIOD = 80;

    uint64_t mRuntimeId;
    Vector3f mPosition;
    Vector3f mRotation;
    Vector3f mMotion;
    ActorFlags mFlags;
    ActorAttributes mAttributes;
    bool mOnGround = false;
    float mFallDistance = 0.0f;
    float mHighestPosition = 0.0f;
    ExperienceManager mExperience;
    MobEffectManager mEffects;
    int mFoodTickTimer = 0;
    bool mHungerEnabled = true;
    bool mIsDead = false;
};
