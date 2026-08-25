#pragma once

#include "Actor/Actor.h"
#include "Actor/DynamicPropertyValue.h"
#include "Core/Math/Vector3f.h"
#include "Core/NBT/Tag.h"
#include "Protocol/Types/ItemStack.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct CustomActorDefinition;
class ServerNetworkHandler;
class ServerPlayer;

struct ProjectileData {
    float mBaseDamage = 0.0f;
    bool mCritical = false;
    int32_t mPunchLevel = 0;
    int32_t mFlameTicks = 0;
    int32_t mLoyaltyLevel = 0;
    int32_t mImpalingLevel = 0;
    int32_t mPiercingLevel = 0;
    bool mChanneling = false;
    bool mReturning = false;
    bool mHadCollision = false;
    bool mPickupCreativeOnly = false;
    ItemStack mPickupItem;
    int32_t mFavoredSlot = -1;
    Tag mFireworkData;
    int32_t mFireworkLifetime = 0;
    int32_t mFireworkAge = 0;
    bool mFireworkAttached = false;
};

class ServerActor : public Actor {
public:
    ServerActor(uint64_t runtimeId, const std::string &identifier);

    void tick(ServerNetworkHandler &owner);

    bool hurt(ServerNetworkHandler &owner, float amount, ServerPlayer *source);

    void tickFire(ServerNetworkHandler &owner);

    void tickSunlightBurn(ServerNetworkHandler &owner);

    void _tickPhysics(ServerNetworkHandler &owner);

    const char *getIdentifier() const override { return mIdentifier.c_str(); }

    const std::string &getTypeId() const { return mIdentifier; }

    void setDefinition(const CustomActorDefinition *definition) { mDefinition = definition; }

    const CustomActorDefinition *getDefinition() const { return mDefinition; }

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

    ProjectileData &getProjectileData() { return mProjectileData; }

    const ProjectileData &getProjectileData() const { return mProjectileData; }

    int32_t getLifetimeTicks() const { return mLifetimeTicks; }

    void addLifetimeTick() { mLifetimeTicks++; }

    int32_t getDeathTicks() const { return mDeathTicks; }

    void addDeathTick() { mDeathTicks++; }

    const std::string &getNameTag() const { return mNameTag; }

    void setNameTag(const std::string &nameTag) { mNameTag = nameTag; }

    bool shouldSave() const { return isAlive() && !mIsProjectile && !hasOwnerPlayer(); }

    Tag saveNbt() const;

    void loadNbt(const Tag &data);

private:
    std::string mIdentifier;
    const CustomActorDefinition *mDefinition = nullptr;
    bool mIsProjectile = false;
    int64_t mOwnerUniqueId = -1;
    uint32_t mOwnerPlayerHandle = 0xFFFFFFFF;
    int32_t mLifetimeTicks = 0;
    int32_t mDeathTicks = 0;
    ProjectileData mProjectileData;
    std::string mNameTag;

    std::unordered_set<std::string> mTags;
    std::unordered_map<std::string, int32_t> mIntProperties;
    std::unordered_map<std::string, float> mFloatProperties;
    std::unordered_map<std::string, DynamicPropertyValue> mDynamicProperties;
};
