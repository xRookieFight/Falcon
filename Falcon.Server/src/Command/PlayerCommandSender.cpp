#include "Command/PlayerCommandSender.h"

#include "Actor/ServerPlayer.h"
#include "Network/ServerNetworkHandler.h"

PlayerCommandSender::PlayerCommandSender(ServerNetworkHandler &handler, ServerPlayer &player,
                                         const CommandOriginData &origin)
        : mHandler(handler), mPlayer(player), mOrigin(origin) {}

const std::string &PlayerCommandSender::getSenderName() const {
    return mPlayer.getName();
}

void PlayerCommandSender::sendMessage(const std::string &message) {
    mPlayer.sendMessage(message);
}

void PlayerCommandSender::sendTranslation(const std::string &key, const std::vector<std::string> &parameters) {
    mPlayer.sendTranslation(key, parameters);
}

CommandPermission PlayerCommandSender::getCommandPermission() const {
    return mPlayer.getCommandPermission();
}
