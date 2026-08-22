#pragma once

#include "Core/Math/Vector3f.h"
#include "Entity/EntityAttributes.h"
#include "Entity/EntityFlags.h"
#include "Entity/ExperienceManager.h"

#include <cstdint>
#include <string>

class Entity {
public:
    explicit Entity(uint64_t runtimeId);

    virtual ~Entity() = default;

    virtual const char *getIdentifier() const = 0;

    uint64_t getRuntimeId() const { return mRuntimeId; }

    int64_t getUniqueId() const { return (int64_t) mRuntimeId; }

    const Vector3f &getPosition() const { return mPosition; }

    void setPosition(const Vector3f &position) { mPosition = position; }

    const Vector3f &getRotation() const { return mRotation; }

    void setRotation(const Vector3f &rotation) { mRotation = rotation; }

    const Vector3f &getMotion() const { return mMotion; }

    void setMotion(const Vector3f &motion) { mMotion = motion; }

    EntityFlags &getFlags() { return mFlags; }

    const EntityFlags &getFlags() const { return mFlags; }

    EntityAttributes &getAttributes() { return mAttributes; }

    const EntityAttributes &getAttributes() const { return mAttributes; }

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

protected:
    static const float EXHAUSTION_PER_UNIT;
    static const int FOOD_TICK_PERIOD = 80;

    uint64_t mRuntimeId;
    Vector3f mPosition;
    Vector3f mRotation;
    Vector3f mMotion;
    EntityFlags mFlags;
    EntityAttributes mAttributes;
    bool mOnGround = false;
    float mFallDistance = 0.0f;
    float mHighestPosition = 0.0f;
    ExperienceManager mExperience;
    int mFoodTickTimer = 0;
    bool mHungerEnabled = true;
};
