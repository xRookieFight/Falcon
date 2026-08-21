#pragma once

#include "Network/NetworkIdentifier.h"

#include <string>

// One connected client and where it is in the login sequence.
class ServerPlayer {
public:
    enum class LoginState : int {
        Connecting = 0,
        NetworkSettingsSent = 1,
        LoggedIn = 2,
        ResourcePacksSent = 3,
        StartGameSent = 4,
        Spawned = 5
    };

    ServerPlayer(const NetworkIdentifier &id, uint64_t runtimeId);

    const NetworkIdentifier &getNetworkIdentifier() const { return mId; }

    uint64_t getRuntimeId() const { return mRuntimeId; }

    int64_t getUniqueId() const { return (int64_t) mRuntimeId; }

    LoginState getLoginState() const { return mLoginState; }

    void setLoginState(LoginState state) { mLoginState = state; }

    const std::string &getName() const { return mName; }

    void setName(const std::string &name) { mName = name; }

    const std::string &getUuid() const { return mUuid; }

    void setUuid(const std::string &uuid) { mUuid = uuid; }

    const std::string &getXuid() const { return mXuid; }

    void setXuid(const std::string &xuid) { mXuid = xuid; }

private:
    NetworkIdentifier mId;
    uint64_t mRuntimeId;
    LoginState mLoginState;
    std::string mName;
    std::string mUuid;
    std::string mXuid;
};
