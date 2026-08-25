#pragma once

#include "command/CommandOrigin.h"
#include "protocol/types/CommandOriginData.h"

class ServerNetworkHandler;
class ServerPlayer;

class PlayerCommandOrigin : public CommandOrigin {
public:
    PlayerCommandOrigin(ServerNetworkHandler &handler, ServerPlayer &player, const CommandOriginData &origin);

    const std::string &getSenderName() const override;

    bool isPlayer() const override { return true; }

    ServerPlayer *asPlayer() override { return &mPlayer; }

    void sendMessage(const std::string &message) override;

    void sendTranslation(const std::string &key, const std::vector<std::string> &parameters) override;

    CommandPermission getCommandPermission() const override;

private:
    ServerNetworkHandler &mHandler;
    ServerPlayer &mPlayer;
    CommandOriginData mOrigin;
};
