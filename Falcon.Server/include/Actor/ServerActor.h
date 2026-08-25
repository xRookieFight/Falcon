#pragma once

#include "actor/Actor.h"
#include "actor/DynamicPropertyValue.h"
#include "core/math/Vector3f.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct CustomActorDefinition;

class ServerActor : public Actor {
public:
    ServerActor(uint64_t runtimeId, const std::string &identifier);

    const char *getIdentifier() const override { return mIdentifier.c_str(); }

    const std::string &getTypeId() const { return mIdentifier; }

    void setDefinition(const CustomActorDefinition *definition) { mDefinition = definition; }

    const CustomActorDefinition *getDefinition() const { return mDefinition; }

    bool isAlive() const { return mAlive; }

    void markRemoved() { mAlive = false; }

    void addTag(const std::string &tag) { mTags.insert(tag); }

    bool removeTag(const std::string &tag) { return mTags.erase(tag) != 0; }

    bool hasTag(const std::string &tag) const { return mTags.count(tag) != 0; }

    const std::unordered_set<std::string> &getTags() const { return mTags; }

    void setIntProperty(const std::string &name, int32_t value) { mIntProperties[name] = value; }

    void setFloatProperty(const std::string &name, float value) { mFloatProperties[name] = value; }

    bool hasIntProperty(const std::string &name) const { return mIntProperties.count(name) != 0; }

    bool hasFloatProperty(const std::string &name) const { return mFloatProperties.count(name) != 0; }

    int32_t getIntProperty(const std::string &name, int32_t fallback = 0) const;

    float getFloatProperty(const std::string &name, float fallback = 0.0f) const;

    std::unordered_map<std::string, int32_t> &getIntProperties() { return mIntProperties; }

    std::unordered_map<std::string, float> &getFloatProperties() { return mFloatProperties; }

    std::unordered_map<std::string, DynamicPropertyValue> &getDynamicProperties() { return mDynamicProperties; }

    void setOwnerUniqueId(int64_t owner) { mOwnerUniqueId = owner; }

    int64_t getOwnerUniqueId() const { return mOwnerUniqueId; }

    void setOwnerPlayerHandle(uint32_t handle) { mOwnerPlayerHandle = handle; }

    uint32_t getOwnerPlayerHandle() const { return mOwnerPlayerHandle; }

    bool hasOwnerPlayer() const { return mOwnerPlayerHandle != 0xFFFFFFFF; }

    bool isProjectile() const { return mIsProjectile; }

    void setProjectile(bool value) { mIsProjectile = value; }

    int32_t getLifetimeTicks() const { return mLifetimeTicks; }

    void addLifetimeTick() { mLifetimeTicks++; }

    float getHealth() const { return mHealth; }

    void setHealth(float health) { mHealth = health < 0.0f ? 0.0f : health; }

    float getMaxHealth() const { return mMaxHealth; }

    void setMaxHealth(float maxHealth) { mMaxHealth = maxHealth; }

    const std::string &getNameTag() const { return mNameTag; }

    void setNameTag(const std::string &nameTag) { mNameTag = nameTag; }

private:
    std::string mIdentifier;
    const CustomActorDefinition *mDefinition = nullptr;
    bool mAlive = true;
    bool mIsProjectile = false;
    int64_t mOwnerUniqueId = -1;
    uint32_t mOwnerPlayerHandle = 0xFFFFFFFF;
    int32_t mLifetimeTicks = 0;
    float mHealth = 20.0f;
    float mMaxHealth = 20.0f;
    std::string mNameTag;

    std::unordered_set<std::string> mTags;
    std::unordered_map<std::string, int32_t> mIntProperties;
    std::unordered_map<std::string, float> mFloatProperties;
    std::unordered_map<std::string, DynamicPropertyValue> mDynamicProperties;
};
