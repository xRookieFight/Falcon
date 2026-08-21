#include "Command/PlayerCommandSender.h"

#include "Entity/ServerPlayer.h"
#include "Network/ServerNetworkHandler.h"

PlayerCommandSender::PlayerCommandSender(ServerNetworkHandler &handler, ServerPlayer &player,
                                         const CommandOriginData &origin)
        : mHandler(handler), mPlayer(player), mOrigin(origin) {}

const std::string &PlayerCommandSender::getSenderName() const {
    return mPlayer.getName();
}

void PlayerCommandSender::sendMessage(const std::string &message) {
    mHandler.sendCommandOutput(mPlayer, mOrigin, message);
}
