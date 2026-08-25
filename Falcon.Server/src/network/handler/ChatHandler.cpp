#include "network/handler/ChatHandler.h"

#include "core/debug/BedrockLog.h"
#include "actor/ServerPlayer.h"
#include "network/handler/NetworkHandler.h"
#include "network/handler/ServerNetworkHandler.h"
#include "protocol/packets/TextPacket.h"

void ChatHandler::broadcastChat(ServerNetworkHandler &owner, ServerPlayer &player, const std::string &message) {
    PlayerChatBeforeEvent before(player, message);
    owner.getEventBus().before().mChatSend.emit(before);
    if (before.isCancelled())
        return;

    const std::string &finalMessage = before.mMessage;

    LOG_INFO(LogAreaID::Server, "<%s> %s", player.getName().c_str(), finalMessage.c_str());

    TextPacket chat;
    chat.mType = TextPacket::Type::Chat;
    chat.mSourceName = player.getName();
    chat.mMessage = finalMessage;

    for (auto &entry: owner.getPlayers()) {
        if (entry.second.isSpawned())
            owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), chat, owner.getCodecContext());
    }

    PlayerChatAfterEvent after(player, finalMessage);
    owner.getEventBus().after().mChatSend.emit(after);
}
