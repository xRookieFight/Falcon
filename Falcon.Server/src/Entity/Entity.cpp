#include "Entity/Entity.h"

#include <algorithm>

namespace {
    const char *ATTRIBUTE_HEALTH = "minecraft:health";
    const char *ATTRIBUTE_HUNGER = "minecraft:player.hunger";
    const char *ATTRIBUTE_SATURATION = "minecraft:player.saturation";
    const char *ATTRIBUTE_EXHAUSTION = "minecraft:player.exhaustion";

    const int DIFFICULTY_PEACEFUL = 0;
    const int DIFFICULTY_EASY = 1;
    const int DIFFICULTY_NORMAL = 2;
    const int DIFFICULTY_HARD = 3;
}

const float Entity::EXHAUSTION_PER_UNIT = 4.0f;

Entity::Entity(uint64_t runtimeId) : mRuntimeId(runtimeId) {}

void Entity::setXpAndProgress(int level, float progress) {
    mExperience.setXpAndProgress(level, progress);
    syncExperience();
}

void Entity::addXp(int amount) {
    mExperience.addXp(amount);
    syncExperience();
}

void Entity::addXpLevels(int amount) {
    mExperience.addXpLevels(amount);
    syncExperience();
}

float Entity::getHealth() const {
    return mAttributes.get(ATTRIBUTE_HEALTH);
}

float Entity::getMaxHealth() const {
    return mAttributes.getMaximum(ATTRIBUTE_HEALTH);
}

bool Entity::isAlive() const {
    return getHealth() > 0.0f;
}

float Entity::getFood() const {
    return mAttributes.get(ATTRIBUTE_HUNGER);
}

float Entity::getMaxFood() const {
    return mAttributes.getMaximum(ATTRIBUTE_HUNGER);
}

void Entity::setFood(float food) {
    const float old = mAttributes.get(ATTRIBUTE_HUNGER);
    mAttributes.setClamped(ATTRIBUTE_HUNGER, food);

    const float updated = mAttributes.get(ATTRIBUTE_HUNGER);
    const float bounds[3] = {17.0f, 6.0f, 0.0f};

    for (float bound: bounds) {
        if ((old > bound) != (updated > bound)) {
            mFoodTickTimer = 0;
            break;
        }
    }
}

void Entity::addFood(float amount) {
    setFood(mAttributes.get(ATTRIBUTE_HUNGER) + amount);
}

bool Entity::isHungry() const {
    return getFood() < getMaxFood();
}

bool Entity::canEat() const {
    return isHungry();
}

float Entity::getSaturation() const {
    return mAttributes.get(ATTRIBUTE_SATURATION);
}

void Entity::setSaturation(float saturation) {
    mAttributes.setClamped(ATTRIBUTE_SATURATION, saturation);
}

void Entity::addSaturation(float amount) {
    setSaturation(mAttributes.get(ATTRIBUTE_SATURATION) + amount);
}

float Entity::getExhaustion() const {
    return mAttributes.get(ATTRIBUTE_EXHAUSTION);
}

void Entity::setExhaustion(float exhaustion) {
    mAttributes.setClamped(ATTRIBUTE_EXHAUSTION, exhaustion);
}

void Entity::exhaust(float amount) {
    if (!mHungerEnabled)
        return;

    float exhaustion = getExhaustion() + amount;

    while (exhaustion >= EXHAUSTION_PER_UNIT) {
        exhaustion -= EXHAUSTION_PER_UNIT;

        const float saturation = getSaturation();
        if (saturation > 0.0f) {
            setSaturation(std::max(0.0f, saturation - 1.0f));
            continue;
        }

        const float food = getFood();
        if (food > 0.0f)
            setFood(std::max(0.0f, food - 1.0f));
    }

    setExhaustion(exhaustion);
}

void Entity::setFoodTickTimer(int foodTickTimer) {
    mFoodTickTimer = std::max(0, foodTickTimer);
}

void Entity::consumeFood(int nutrition, float saturation) {
    addFood((float) nutrition);
    addSaturation(saturation);
}

void Entity::resetHungerAndExperience() {
    mAttributes.setClamped(ATTRIBUTE_HUNGER, getMaxFood());
    mAttributes.setClamped(ATTRIBUTE_SATURATION, mAttributes.getMaximum(ATTRIBUTE_SATURATION));
    mAttributes.setClamped(ATTRIBUTE_EXHAUSTION, 0.0f);
    mFoodTickTimer = 0;

    mExperience.reset();
    syncExperience();
}

bool Entity::tickHunger(int tickDiff, int difficulty) {
    if (!isAlive() || !mHungerEnabled)
        return false;

    const float previousFood = getFood();
    const float previousSaturation = getSaturation();
    const float previousExhaustion = getExhaustion();
    const float previousHealth = getHealth();

    float food = previousFood;
    const float maxHealth = getMaxHealth();

    mFoodTickTimer += tickDiff;
    if (mFoodTickTimer >= FOOD_TICK_PERIOD)
        mFoodTickTimer = 0;

    if (difficulty == DIFFICULTY_PEACEFUL && mFoodTickTimer % 10 == 0) {
        if (food < getMaxFood()) {
            addFood(1.0f);
            food = getFood();
        }

        if (mFoodTickTimer % 20 == 0 && getHealth() < maxHealth)
            mAttributes.setClamped(ATTRIBUTE_HEALTH, getHealth() + 1.0f);
    }

    if (mFoodTickTimer == 0) {
        if (food >= 18.0f) {
            if (getHealth() < maxHealth) {
                mAttributes.setClamped(ATTRIBUTE_HEALTH, getHealth() + 1.0f);
                exhaust(6.0f);
            }
        } else if (food <= 0.0f) {
            const float health = getHealth();
            const bool starve = (difficulty == DIFFICULTY_EASY && health > 10.0f)
                                || (difficulty == DIFFICULTY_NORMAL && health > 1.0f)
                                || difficulty == DIFFICULTY_HARD;

            if (starve)
                mAttributes.setClamped(ATTRIBUTE_HEALTH, health - 1.0f);
        }
    }

    if (getFood() <= 6.0f)
        mFlags.set(EntityFlag::Sprinting, false);

    return getFood() != previousFood
           || getSaturation() != previousSaturation
           || getExhaustion() != previousExhaustion
           || getHealth() != previousHealth;
}
