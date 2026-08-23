#pragma once

#include "Actor/Actor.h"
#include "Core/Math/Vector3i.h"
#include "Core/NBT/Tag.h"
#include "Inventory/InventoryManager.h"
#include "Inventory/PlayerInventory.h"
#include "Network/NetworkIdentifier.h"
#include "Network/PacketSender.h"
#include "Protocol/PacketCodecContext.h"
#include "Protocol/Types/AdventureSettingData.h"
#include "Protocol/Types/SerializedSkin.h"
#include "Actor/MobEffect.h"

#include <string>
#include <vector>
#include <unordered_set>

class ServerPlayer : public Actor {
public:
    enum class LoginState : int {
        Connecting = 0,
        NetworkSettingsSent = 1,
        LoggedIn = 2,
        ResourcePacksSent = 3,
        StartGameSent = 4,
        Spawned = 5
    };

    ServerPlayer(const NetworkIdentifier &id, uint64_t runtimeId, PacketSender *sender = nullptr);

    const char *getIdentifier() const override { return "minecraft:player"; }

    const NetworkIdentifier &getNetworkIdentifier() const { return mId; }

    LoginState getLoginState() const { return mLoginState; }

    void setLoginState(LoginState state) { mLoginState = state; }

    bool isSpawned() const { return mLoginState == LoginState::Spawned; }

    const std::string &getName() const { return mName; }

    void setName(const std::string &name) { mName = name; }

    const std::string &getUuid() const { return mUuid; }

    void setUuid(const std::string &uuid) { mUuid = uuid; }

    const std::string &getXuid() const { return mXuid; }

    void setXuid(const std::string &xuid) { mXuid = xuid; }

    const SerializedSkin &getSkin() const { return mSkin; }

    void setSkin(const SerializedSkin &skin) { mSkin = skin; }

    int getBuildPlatform() const { return mBuildPlatform; }

    void setBuildPlatform(int platform) { mBuildPlatform = platform; }

    int32_t getGameType() const { return mGameType; }

    void setGameType(int32_t gameType) { mGameType = gameType; }

    bool isOp() const { return mIsOp; }

    void setOp(bool isOp) { mIsOp = isOp; }

    bool isFlying() const { return mFlying; }

    void setFlying(bool flying) {
        if (mFlying == flying)
            return;

        resetFallDistance();
        mFlying = flying;
    }

    CommandPermission getCommandPermission() const {
        return mIsOp ? CommandPermission::GameDirectors : CommandPermission::Any;
    }

    int64_t getFirstPlayed() const { return mFirstPlayed; }

    void setFirstPlayed(int64_t firstPlayed) { mFirstPlayed = firstPlayed; }

    PlayerInventory &getInventory() { return mInventory; }

    const PlayerInventory &getInventory() const { return mInventory; }

    InventoryManager &getInventoryManager() { return mInventoryManager; }

    PacketSender *getPacketSender() const { return mSender; }

    void setEffectsNetworkReady(bool ready) { mEffectsNetworkReady = ready; }

    void syncEffects();

    Tag saveNbt(const std::string &levelName) const;

    void loadNbt(const Tag &data, const PacketCodecContext &context);

    void sendMessage(const std::string &message);

    void sendTranslation(const std::string &key, const std::vector<std::string> &parameters = {});

    void sendTip(const std::string &message);

    void sendPopup(const std::string &message);

    void sendTitle(const std::string &title, const std::string &subtitle = "", int32_t fadeInTime = 10,
                   int32_t stayTime = 70, int32_t fadeOutTime = 20);

    bool hasPendingMove() const { return mHasPendingMove; }

    const Vector3f &getPendingMovePosition() const { return mPendingMovePosition; }

    const Vector3f &getPendingMoveRotation() const { return mPendingMoveRotation; }

    void queueMove(const Vector3f &position, const Vector3f &rotation, uint64_t clientTick, bool jumping) {
        mPendingMovePosition = position;
        mPendingMoveRotation = rotation;
        mPendingMoveClientTick = clientTick;
        mPendingMoveJumping = jumping;
        mHasPendingMove = true;
    }

    uint64_t getPendingMoveClientTick() const { return mPendingMoveClientTick; }

    bool isPendingMoveJumping() const { return mPendingMoveJumping; }

    void clearPendingMove() { mHasPendingMove = false; }

    int64_t getItemUseStartTick() const { return mItemUseStartTick; }

    void setItemUseStartTick(int64_t tick) { mItemUseStartTick = tick; }

    bool isBreakingBlock() const { return mIsBreakingBlock; }

    const Vector3i &getBreakingBlockPosition() const { return mBreakingBlockPosition; }

    int32_t getBreakingFace() const { return mBreakingFace; }

    double getBreakProgress() const { return mBreakProgress; }

    void addBreakProgress(double amount) { mBreakProgress += amount; }

    void startBreakingBlock(const Vector3i &position, int32_t face) {
        mIsBreakingBlock = true;
        mBreakingBlockPosition = position;
        mBreakingFace = face;
        mBreakProgress = 0.0;
        mBreakingFxTicker = 0;
    }

    void stopBreakingBlock() {
        mIsBreakingBlock = false;
        mBreakProgress = 0.0;
    }

    int getBreakingFxTicker() { return mBreakingFxTicker++; }

    bool hasSeenNonZeroClientTick() const { return mSawNonZeroClientTick; }

    void markNonZeroClientTickSeen() { mSawNonZeroClientTick = true; }

    bool hasForceMoveSync() const { return mForceMoveSync; }

    void setForceMoveSync(bool forceMoveSync) { mForceMoveSync = forceMoveSync; }

    std::unordered_set<int64_t> &getSentChunks() { return mSentChunks; }

    size_t getSentChunkCount() const { return mSentChunkCount; }

    void addSentChunkCount(size_t count) { mSentChunkCount += count; }

    bool hasSpawnChunksReady() const { return mSpawnChunksReady; }

    void setSpawnChunksReady(bool ready) { mSpawnChunksReady = ready; }

    bool hasChunkPosition() const { return mHasChunkPosition; }

    int32_t getLastChunkX() const { return mLastChunkX; }

    int32_t getLastChunkZ() const { return mLastChunkZ; }

    void setLastChunkPosition(int32_t chunkX, int32_t chunkZ) {
        mLastChunkX = chunkX;
        mLastChunkZ = chunkZ;
        mHasChunkPosition = true;
    }

private:
    NetworkIdentifier mId;
    LoginState mLoginState;
    std::string mName;
    std::string mUuid;
    std::string mXuid;
    int mBuildPlatform = -1;
    bool mForceMoveSync = false;
    int64_t mItemUseStartTick = 0;
    bool mHasPendingMove = false;
    Vector3f mPendingMovePosition;
    Vector3f mPendingMoveRotation;
    uint64_t mPendingMoveClientTick = 0;
    bool mPendingMoveJumping = false;
    bool mIsBreakingBlock = false;
    Vector3i mBreakingBlockPosition;
    int32_t mBreakingFace = 0;
    double mBreakProgress = 0.0;
    int mBreakingFxTicker = 0;
    bool mSawNonZeroClientTick = false;
    std::unordered_set<int64_t> mSentChunks;
    size_t mSentChunkCount = 0;
    bool mSpawnChunksReady = false;
    int32_t mLastChunkX = 0;
    int32_t mLastChunkZ = 0;
    bool mHasChunkPosition = false;
    SerializedSkin mSkin;
    int32_t mGameType = 0;
    int64_t mFirstPlayed = 0;
    bool mIsOp = false;
    bool mFlying = false;
    PlayerInventory mInventory;
    InventoryManager mInventoryManager;
    PacketSender *mSender;
    bool mEffectsNetworkReady = false;
};
