#pragma once

#include "Core/NBT/Tag.h"
#include "Entity/Entity.h"
#include "Network/NetworkIdentifier.h"
#include "Network/PacketSender.h"
#include "Protocol/Types/AdventureSettingData.h"

#include <string>

// One connected client and where it is in the login sequence.
class ServerPlayer : public Entity {
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

    int32_t getGameType() const { return mGameType; }

    void setGameType(int32_t gameType) { mGameType = gameType; }

    bool isOp() const { return mIsOp; }

    void setOp(bool isOp) { mIsOp = isOp; }

    CommandPermission getCommandPermission() const {
        return mIsOp ? CommandPermission::GameDirectors : CommandPermission::Any;
    }

    int64_t getFirstPlayed() const { return mFirstPlayed; }

    void setFirstPlayed(int64_t firstPlayed) { mFirstPlayed = firstPlayed; }

    Tag saveNbt(const std::string &levelName) const;

    void loadNbt(const Tag &data);

    void sendMessage(const std::string &message);

    void sendTip(const std::string &message);

    void sendPopup(const std::string &message);

    void sendTitle(const std::string &title, const std::string &subtitle = "", int32_t fadeInTime = 10,
                   int32_t stayTime = 70, int32_t fadeOutTime = 20);

private:
    NetworkIdentifier mId;
    LoginState mLoginState;
    std::string mName;
    std::string mUuid;
    std::string mXuid;
    int32_t mGameType = 0;
    int64_t mFirstPlayed = 0;
    bool mIsOp = false;
    PacketSender *mSender;
};
