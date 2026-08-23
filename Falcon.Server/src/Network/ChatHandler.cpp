#include "Network/ChatHandler.h"

#include "Core/Debug/BedrockLog.h"
#include "Actor/ServerPlayer.h"
#include "Network/NetworkHandler.h"
#include "Network/ServerNetworkHandler.h"
#include "Protocol/Packets/TextPacket.h"

void ChatHandler::broadcastChat(ServerNetworkHandler &owner, ServerPlayer &player, const std::string &message) {
    LOG_INFO(LogAreaID::Server, "<%s> %s", player.getName().c_str(), message.c_str());

    TextPacket chat;
    chat.mType = TextPacket::Type::Chat;
    chat.mSourceName = player.getName();
    chat.mMessage = message;

    for (auto &entry: owner.getPlayers()) {
        if (entry.second.isSpawned())
            owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), chat, owner.getCodecContext());
    }
}
